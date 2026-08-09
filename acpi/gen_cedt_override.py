#!/usr/bin/env python3
#########################################################################
# File Name: gen_cedt_override.py
# Desc: Generate an overridden CEDT ACPI table binary from a known-good
#       base cedt.dsl, resizing one existing CFMWS window in place.
#       Used with build_cedt_override_initrd.sh to work around BIOS bugs
#       that drop/misconfigure CXL Fixed Memory Window entries (see
#       AMD_CEDT_DCD_ISSUE_SUMMARY.md for the motivating issue).
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
#########################################################################
"""Patch the size of an existing CFMWS window in a CEDT ACPI table.

Takes a base CEDT disassembly (as produced by `iasl -d cedt.dat`), finds the
CXL Fixed Memory Window Structure (CFMWS) whose "Window base address" matches
--window-base, rewrites its "Window size" field, bumps OEM Revision (so the
result is recognized as an initrd table override upgrade), recompiles with
`iasl -sa`, and independently re-verifies the compiled output.

Requires the ACPICA `iasl` tool on PATH for anything beyond --list.
"""

import argparse
import hashlib
import re
import shutil
import subprocess
import sys
from pathlib import Path

FIELD_RE = re.compile(
    r"^\[(?P<hoff>[0-9A-Fa-f]+)h\s+(?P<doff>\d+)\s+(?P<blen>[0-9A-Fa-f]+)h\]\s*"
    r"(?P<name>.+?)\s*:\s*(?P<value>.*?)\s*$"
)

UNIT_MULT = {"K": 1 << 10, "M": 1 << 20, "G": 1 << 30, "T": 1 << 40}


class CedtError(Exception):
    def __init__(self, exit_code, message):
        super().__init__(message)
        self.exit_code = exit_code


def parse_dsl(lines):
    """Group iasl -d field lines into subtable records by 'Subtable Type'.

    Returns a list of dicts: {'type': int|None, 'fields': {name: {line, value, blen}}}.
    The first record (type None) is the ACPI table header.
    """
    subtables = []
    current = {"type": None, "fields": {}}
    for idx, line in enumerate(lines):
        m = FIELD_RE.match(line)
        if not m:
            continue
        name = m.group("name").strip()
        value = m.group("value").strip()
        blen = int(m.group("blen"), 16)
        if name == "Subtable Type":
            if current["fields"]:
                subtables.append(current)
            current = {"type": int(value.split()[0], 16), "fields": {}}
        current["fields"][name] = {"line": idx, "value": value, "blen": blen}
    if current["fields"]:
        subtables.append(current)
    return subtables


def hexfield_int(field):
    return int(field["value"].split()[0].strip('"'), 16)


def cfmws_entries(subtables):
    return [st for st in subtables if st["type"] == 1]


def load_subtables(dsl_path):
    text = Path(dsl_path).read_text()
    lines = text.splitlines()
    subtables = parse_dsl(lines)
    if not subtables or subtables[0]["type"] is not None:
        raise CedtError(2, f"malformed base dsl (no ACPI table header found): {dsl_path}")
    return lines, subtables


def human_size(n):
    for unit, mult in (("T", 1 << 40), ("G", 1 << 30), ("M", 1 << 20), ("K", 1 << 10)):
        if n % mult == 0 and n >= mult:
            return f"{n // mult}{unit}"
    return str(n)


def parse_size(s):
    s = s.strip()
    m = re.fullmatch(r"(\d+(?:\.\d+)?)\s*([KMGTkmgt])[iI]?[bB]?", s)
    if m:
        num = float(m.group(1))
        mult = UNIT_MULT[m.group(2).upper()]
        val = num * mult
        if val != int(val):
            raise argparse.ArgumentTypeError(f"size {s!r} is not an integral number of bytes")
        return int(val)
    try:
        return int(s, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(f"cannot parse size {s!r} (use e.g. 4G, 0x100000000, or a decimal byte count)")


def parse_int_auto(s):
    try:
        return int(s, 0)
    except ValueError:
        raise argparse.ArgumentTypeError(f"cannot parse integer {s!r}")


def do_list(args):
    _, subtables = load_subtables(args.base_dsl)
    entries = cfmws_entries(subtables)
    if not entries:
        print("(no CFMWS entries found in base dsl)")
        return 0
    for i, st in enumerate(entries):
        f = st["fields"]
        base = hexfield_int(f["Window base address"]) if "Window base address" in f else None
        size = hexfield_int(f["Window size"]) if "Window size" in f else None
        restr = hexfield_int(f["Restrictions"]) if "Restrictions" in f else None
        qtgid = hexfield_int(f["QtgId"]) if "QtgId" in f else None
        target = hexfield_int(f["First Target"]) if "First Target" in f else None
        print(
            f"CFMWS[{i}]: base=0x{base:x} size=0x{size:x} ({human_size(size)}) "
            f"restrictions=0x{restr:04x} qtgid=0x{qtgid:04x} first_target=0x{target:08x}"
        )
    return 0


def replace_field_value(lines, field, new_hex):
    line = lines[field["line"]]
    m = FIELD_RE.match(line)
    lines[field["line"]] = line[: m.start("value")] + new_hex


def run_iasl(iasl_bin, args_list, cwd):
    proc = subprocess.run(
        [iasl_bin] + args_list, cwd=cwd, capture_output=True, text=True
    )
    return proc


def do_override(args):
    lines, subtables = load_subtables(args.base_dsl)
    header = subtables[0]
    entries = cfmws_entries(subtables)

    match = next(
        (
            st
            for st in entries
            if "Window base address" in st["fields"]
            and hexfield_int(st["fields"]["Window base address"]) == args.window_base
        ),
        None,
    )
    if match is None:
        bases = [
            hex(hexfield_int(st["fields"]["Window base address"]))
            for st in entries
            if "Window base address" in st["fields"]
        ]
        bases_str = ", ".join(bases) if bases else "(none)"
        raise CedtError(
            3,
            f"no CFMWS entry with Window base address 0x{args.window_base:x} in "
            f"{args.base_dsl}. Present base addresses: {bases_str}",
        )

    size_field = match["fields"]["Window size"]
    old_size = hexfield_int(size_field)
    new_size = args.window_size
    if new_size >= (1 << (size_field["blen"] * 8)):
        raise CedtError(
            1, f"window size 0x{new_size:x} does not fit in {size_field['blen']} bytes"
        )

    oem_field = header["fields"]["Oem Revision"]
    old_oem = hexfield_int(oem_field)
    new_oem = args.oem_revision if args.oem_revision is not None else old_oem + 1
    if new_oem <= old_oem:
        raise CedtError(
            1,
            f"--oem-revision 0x{new_oem:x} must be strictly greater than the base "
            f"table's current OEM Revision 0x{old_oem:x} (initrd override upgrade "
            "semantics require a higher revision)",
        )

    replace_field_value(lines, size_field, format(new_size, f"0{size_field['blen']*2}X"))
    replace_field_value(lines, oem_field, format(new_oem, f"0{oem_field['blen']*2}X"))

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    stem = Path(args.base_dsl).stem
    override_dsl_path = outdir / f"{stem}_override.dsl"
    override_dsl_path.write_text("\n".join(lines) + "\n")

    iasl_bin = shutil.which(args.iasl_path)
    if iasl_bin is None:
        raise CedtError(
            4,
            f"'{args.iasl_path}' not found on PATH. Install ACPICA tools "
            "(e.g. 'sudo apt install acpica-tools').",
        )

    proc = run_iasl(iasl_bin, ["-sa", override_dsl_path.name], cwd=outdir)
    if proc.returncode != 0:
        raise CedtError(
            5,
            f"iasl -sa failed compiling {override_dsl_path}:\n{proc.stdout}\n{proc.stderr}",
        )

    compiled_aml = outdir / f"{stem}_override.aml"
    if not compiled_aml.exists():
        present = ", ".join(p.name for p in outdir.iterdir())
        raise CedtError(
            5,
            f"expected iasl output {compiled_aml} not found after compile. "
            f"Files present in {outdir}: {present}",
        )

    verify_aml = outdir / f"{stem}_verify.aml"
    shutil.copy(compiled_aml, verify_aml)
    proc = run_iasl(iasl_bin, ["-d", verify_aml.name], cwd=outdir)
    verify_dsl = outdir / f"{stem}_verify.dsl"
    if proc.returncode != 0 or not verify_dsl.exists():
        raise CedtError(
            6,
            f"could not re-disassemble compiled output for verification:\n"
            f"{proc.stdout}\n{proc.stderr}",
        )

    v_lines, v_subtables = load_subtables(verify_dsl)
    v_header = v_subtables[0]
    v_entries = cfmws_entries(v_subtables)
    v_match = next(
        (
            st
            for st in v_entries
            if "Window base address" in st["fields"]
            and hexfield_int(st["fields"]["Window base address"]) == args.window_base
        ),
        None,
    )
    problems = []
    if v_match is None:
        problems.append("CFMWS entry at target base address is missing from compiled output")
    else:
        got_size = hexfield_int(v_match["fields"]["Window size"])
        if got_size != new_size:
            problems.append(f"Window size mismatch: expected 0x{new_size:x}, got 0x{got_size:x}")
    orig_table_len = hexfield_int(header["fields"]["Table Length"])
    got_table_len = hexfield_int(v_header["fields"]["Table Length"])
    if got_table_len != orig_table_len:
        problems.append(
            f"Table Length changed: expected 0x{orig_table_len:x}, got 0x{got_table_len:x}"
        )
    got_oem = hexfield_int(v_header["fields"]["Oem Revision"])
    if got_oem != new_oem:
        problems.append(f"Oem Revision mismatch: expected 0x{new_oem:x}, got 0x{got_oem:x}")

    compiled_bytes = compiled_aml.read_bytes()
    if sum(compiled_bytes) & 0xFF != 0:
        problems.append("ACPI checksum invariant violated (sum of table bytes mod 256 != 0)")
    sig = header["fields"]["Signature"]["value"].split()[0].strip('"').encode()
    if compiled_bytes[:4] != sig:
        problems.append(f"table signature mismatch: expected {sig!r}, got {compiled_bytes[:4]!r}")

    if problems:
        raise CedtError(6, "post-compile verification failed:\n  " + "\n  ".join(problems))

    output_name = args.output or "cedt_override.dat"
    output_path = Path(output_name)
    if not output_path.is_absolute():
        output_path = outdir / output_path
    if output_path.exists() and not args.force:
        raise CedtError(7, f"output {output_path} already exists (use --force to overwrite)")
    shutil.copy(compiled_aml, output_path)

    if args.no_keep_intermediates:
        for p in (override_dsl_path, verify_dsl, verify_aml, compiled_aml):
            p.unlink(missing_ok=True)
    else:
        verify_aml.unlink(missing_ok=True)

    digest = hashlib.sha256(output_path.read_bytes()).hexdigest()
    print(f"base dsl:        {args.base_dsl}")
    print(f"matched base:    0x{args.window_base:x}")
    print(f"window size:     0x{old_size:x} ({human_size(old_size)}) -> 0x{new_size:x} ({human_size(new_size)})")
    print(f"oem revision:    0x{old_oem:x} -> 0x{new_oem:x}")
    print(f"output:          {output_path} ({len(compiled_bytes)} bytes, sha256={digest})")
    if not args.no_keep_intermediates:
        print(f"intermediates:   {override_dsl_path}, {verify_dsl}")
    return 0


def build_parser():
    p = argparse.ArgumentParser(
        description="Patch the size of an existing CFMWS window in a base CEDT dsl and "
        "recompile it into an override table binary."
    )
    p.add_argument("--base-dsl", required=True, help="path to base cedt.dsl (iasl -d output)")
    p.add_argument("--list", action="store_true", help="list CFMWS entries in base dsl and exit (no iasl needed)")
    p.add_argument("--window-base", type=parse_int_auto, help="target CFMWS 'Window base address' to match, e.g. 0x4060000000")
    p.add_argument("--window-size", type=parse_size, help="new window size, e.g. 4G, 0x100000000, or a decimal byte count")
    p.add_argument("--oem-revision", type=parse_int_auto, default=None, help="explicit OEM Revision (default: base value + 1)")
    p.add_argument("--outdir", default=".", help="directory for output + intermediate files (default: .)")
    p.add_argument("--output", default=None, help="output table filename (default: cedt_override.dat)")
    p.add_argument("--iasl-path", default="iasl", help="iasl binary to use (default: iasl on PATH)")
    p.add_argument("--force", action="store_true", help="overwrite output if it already exists")
    p.add_argument("--no-keep-intermediates", action="store_true", help="remove intermediate .dsl/.aml files after success")
    p.add_argument("-v", "--verbose", action="store_true")
    return p


def main(argv=None):
    args = build_parser().parse_args(argv)

    if args.list:
        try:
            return do_list(args)
        except CedtError as e:
            print(f"error: {e}", file=sys.stderr)
            return e.exit_code

    if args.window_base is None or args.window_size is None:
        build_parser().error("--window-base and --window-size are required unless --list is given")

    try:
        return do_override(args)
    except CedtError as e:
        print(f"error: {e}", file=sys.stderr)
        return e.exit_code


if __name__ == "__main__":
    sys.exit(main())
