#!/usr/bin/env bash
#########################################################################
# File Name: build_cedt_override_initrd.sh
# Desc: Build an ACPI-table-override initrd carrying a patched CEDT table
#       (see gen_cedt_override.py) and install it as a selectable GRUB
#       boot entry, without touching the default boot entry or rebooting.
#       Implements the mechanism documented in
#       Documentation/admin-guide/acpi/initrd_table_override.rst.
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
#########################################################################
set -euo pipefail

PROG=$(basename "$0")

KERNEL_VERSION=$(uname -r)
KERNEL_IMAGE=""
INITRD_IMAGE=""
BOOT_DIR="/boot"
OUT_INITRD_NAME=""
GRUB_CFG=""
ENTRY_TITLE=""
ENTRY_ID=""
DRY_RUN=0
DO_CLEANUP=0
ASSUME_YES=0
VERBOSE=0
CEDT_TABLE=""

usage() {
    cat <<EOF
Usage: $PROG [options] <cedt-table-file>

Build an override initrd carrying <cedt-table-file> at kernel/firmware/acpi/
and install it as a selectable GRUB boot entry (default entry untouched,
no reboot performed).

Options:
  --kernel-version VER      kernel version to target (default: uname -r; note
                             this is only used as a filename hint -- some
                             distros, e.g. Arch, don't embed it in filenames)
  --kernel-image PATH       explicit vmlinuz path (skips auto-detection)
  --initrd-image PATH       explicit original initrd path (skips auto-detection)
  --boot-dir DIR            default: /boot
  --out-initrd-name NAME    default: initrd-cedt-override-<VER>.img
  --grub-cfg PATH           explicit grub.cfg path (skips auto-detection)
  --entry-title TITLE       default: "CEDT ACPI Override (<VER>)"
  --entry-id ID             default: cedt-override-<VER>
  --cleanup, --uninstall    remove the installed entry + generated initrd
  --dry-run                 print every action, touch nothing (no root needed)
  -y, --yes                 skip interactive confirmation
  -v, --verbose             verbose tracing
  -h, --help                this help

Exit codes: 0 ok, 1 usage, 2 not root, 3 unsupported/undetected bootloader,
4 ambiguous kernel/initrd detection, 5 grub.cfg/mkconfig tool not found,
6 grub regeneration failed, 7 input file is not a CEDT table, 8 nothing to
clean up.
EOF
}

log()  { printf '%s\n' "$*" >&2; }
vlog() { [ "$VERBOSE" -eq 1 ] && printf '+ %s\n' "$*" >&2 || true; }
die()  { local code=$1; shift; log "error: $*"; exit "$code"; }

# Print+run a mutating action, or just print it under --dry-run.
act() {
    if [ "$DRY_RUN" -eq 1 ]; then
        printf '[dry-run] %s\n' "$*"
    else
        vlog "$*"
        "$@"
    fi
}

while [ $# -gt 0 ]; do
    case "$1" in
        --kernel-version) KERNEL_VERSION=$2; shift 2 ;;
        --kernel-image) KERNEL_IMAGE=$2; shift 2 ;;
        --initrd-image) INITRD_IMAGE=$2; shift 2 ;;
        --boot-dir) BOOT_DIR=$2; shift 2 ;;
        --out-initrd-name) OUT_INITRD_NAME=$2; shift 2 ;;
        --grub-cfg) GRUB_CFG=$2; shift 2 ;;
        --entry-title) ENTRY_TITLE=$2; shift 2 ;;
        --entry-id) ENTRY_ID=$2; shift 2 ;;
        --cleanup|--uninstall) DO_CLEANUP=1; shift ;;
        --dry-run) DRY_RUN=1; shift ;;
        -y|--yes) ASSUME_YES=1; shift ;;
        -v|--verbose) VERBOSE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        --) shift; break ;;
        -*) die 1 "unknown option: $1" ;;
        *) break ;;
    esac
done

[ -n "$OUT_INITRD_NAME" ] || OUT_INITRD_NAME="initrd-cedt-override-${KERNEL_VERSION}.img"
[ -n "$ENTRY_TITLE" ] || ENTRY_TITLE="CEDT ACPI Override (${KERNEL_VERSION})"
[ -n "$ENTRY_ID" ] || ENTRY_ID="cedt-override-${KERNEL_VERSION}"
GRUB_FRAGMENT="/etc/grub.d/41_custom_cedt_override"

if [ "$DO_CLEANUP" -eq 0 ]; then
    [ $# -eq 1 ] || { usage; die 1 "expected exactly one <cedt-table-file> argument"; }
    CEDT_TABLE=$1
fi

# --- root check (dry-run never touches the system, so it's exempt) ---
require_root() {
    if [ "$DRY_RUN" -eq 0 ] && [ "$(id -u)" -ne 0 ]; then
        die 2 "must run as root (writes to $BOOT_DIR and /etc/grub.d); re-run with sudo, or use --dry-run"
    fi
}

# --- validate input table ---
validate_cedt_table() {
    local sig
    sig=$(head -c4 "$CEDT_TABLE" 2>/dev/null || true)
    [ "$sig" = "CEDT" ] || die 7 "$CEDT_TABLE does not start with the 'CEDT' ACPI signature"
    if command -v iasl >/dev/null 2>&1; then
        local tmp
        tmp=$(mktemp -d)
        cp "$CEDT_TABLE" "$tmp/cedt.dat"
        if (cd "$tmp" && iasl -d cedt.dat) >/dev/null 2>&1; then
            log "parsed $CEDT_TABLE for confirmation:"
            grep -E "Window base address|Window size|Oem Revision" "$tmp/cedt.dsl" | sed 's/^/  /' >&2 || true
        fi
        rm -rf "$tmp"
    fi
}

# --- kernel / initrd auto-detection ---
detect_kernel_initrd() {
    if [ -n "$KERNEL_IMAGE" ] && [ -n "$INITRD_IMAGE" ]; then
        [ -f "$KERNEL_IMAGE" ] || die 4 "--kernel-image $KERNEL_IMAGE not found"
        [ -f "$INITRD_IMAGE" ] || die 4 "--initrd-image $INITRD_IMAGE not found"
        return
    fi

    local vmlinuz_candidates initrd_candidates
    vmlinuz_candidates=$(find "$BOOT_DIR" -maxdepth 1 -type f -name 'vmlinuz*' 2>/dev/null | sort)
    initrd_candidates=$(find "$BOOT_DIR" -maxdepth 1 -type f \
        \( -name 'initrd.img-*' -o -name 'initramfs-*' -o -name 'initrd-*' \) 2>/dev/null \
        | grep -vi fallback | sort)

    if [ -n "$KERNEL_VERSION" ]; then
        local vf if_
        vf=$(printf '%s\n' "$vmlinuz_candidates" | grep -F "$KERNEL_VERSION" || true)
        if_=$(printf '%s\n' "$initrd_candidates" | grep -F "$KERNEL_VERSION" || true)
        [ -n "$vf" ] && vmlinuz_candidates=$vf
        [ -n "$if_" ] && initrd_candidates=$if_
    fi

    if [ -z "$KERNEL_IMAGE" ]; then
        local n
        n=$(printf '%s\n' "$vmlinuz_candidates" | grep -c . || true)
        [ "$n" -eq 1 ] || die 4 "could not uniquely determine kernel image under $BOOT_DIR; candidates:
$vmlinuz_candidates
Pass --kernel-image explicitly."
        KERNEL_IMAGE=$vmlinuz_candidates
    fi
    if [ -z "$INITRD_IMAGE" ]; then
        local n
        n=$(printf '%s\n' "$initrd_candidates" | grep -c . || true)
        [ "$n" -eq 1 ] || die 4 "could not uniquely determine original initrd under $BOOT_DIR; candidates:
$initrd_candidates
Pass --initrd-image explicitly."
        INITRD_IMAGE=$initrd_candidates
    fi
    log "detected kernel image: $KERNEL_IMAGE"
    log "detected original initrd: $INITRD_IMAGE"
}

# --- bootloader detection ---
BOOTLOADER=""  # legacy_grub2 | bls_grubby | systemd_boot

detect_bootloader() {
    if [ -z "$GRUB_CFG" ]; then
        for c in /boot/grub/grub.cfg /boot/grub2/grub.cfg; do
            [ -f "$c" ] && { GRUB_CFG=$c; break; }
        done
    fi

    if command -v bootctl >/dev/null 2>&1 && [ -z "$GRUB_CFG" ]; then
        if bootctl status 2>/dev/null | grep -qi 'systemd-boot' || [ -d /boot/loader/entries ]; then
            BOOTLOADER="systemd_boot"
            return
        fi
    fi

    [ -n "$GRUB_CFG" ] || die 3 "no grub.cfg found (checked /boot/grub/grub.cfg, /boot/grub2/grub.cfg) and no systemd-boot detected either; pass --grub-cfg or handle this bootloader manually"

    if command -v grubby >/dev/null 2>&1 && { grep -qi blscfg "$GRUB_CFG" 2>/dev/null || grep -qi '^GRUB_ENABLE_BLSCFG=true' /etc/default/grub 2>/dev/null; }; then
        BOOTLOADER="bls_grubby"
    elif command -v update-grub >/dev/null 2>&1 || command -v grub-mkconfig >/dev/null 2>&1 || command -v grub2-mkconfig >/dev/null 2>&1; then
        BOOTLOADER="legacy_grub2"
    else
        die 5 "found $GRUB_CFG but no update-grub/grub-mkconfig/grub2-mkconfig/grubby on PATH to regenerate it"
    fi
    log "detected bootloader: $BOOTLOADER (grub.cfg: $GRUB_CFG)"
}

regen_grub_cfg() {
    if command -v update-grub >/dev/null 2>&1; then
        act update-grub
    elif command -v grub2-mkconfig >/dev/null 2>&1; then
        act grub2-mkconfig -o "$GRUB_CFG"
    elif command -v grub-mkconfig >/dev/null 2>&1; then
        act grub-mkconfig -o "$GRUB_CFG"
    else
        die 5 "no grub config regeneration tool found"
    fi
}

# --- build the override initrd (per initrd_table_override.rst's exact recipe) ---
build_override_initrd() {
    local out="$BOOT_DIR/$OUT_INITRD_NAME"
    log "building override initrd: $out"
    if [ "$DRY_RUN" -eq 1 ]; then
        printf '[dry-run] mkdir -p <tmp>/kernel/firmware/acpi; cp %s <tmp>/kernel/firmware/acpi/; find kernel | cpio -H newc --create > %s; cat %s >> %s\n' \
            "$CEDT_TABLE" "$out" "$INITRD_IMAGE" "$out"
        return
    fi
    local workdir
    workdir=$(mktemp -d)
    mkdir -p "$workdir/kernel/firmware/acpi"
    cp "$CEDT_TABLE" "$workdir/kernel/firmware/acpi/"
    ( cd "$workdir" && find kernel | cpio -H newc --create ) > "$out"
    cat "$INITRD_IMAGE" >> "$out"
    rm -rf "$workdir"
    log "wrote $out"
}

backup_file() {
    local f=$1
    [ -f "$f" ] || return 0
    local bak="${f}.bak.$(date +%Y%m%d%H%M%S 2>/dev/null || echo snapshot)"
    act cp -p "$f" "$bak"
    log "backed up $f -> $bak"
}

# --- legacy grub2: clone the current default menuentry, patch title/id/initrd ---
install_legacy_grub2() {
    local kernel_basename
    kernel_basename=$(basename "$KERNEL_IMAGE")

    local patched
    patched=$(python3 - "$GRUB_CFG" "$kernel_basename" "$OUT_INITRD_NAME" "$ENTRY_TITLE" "$ENTRY_ID" <<'PYEOF'
import sys, re

grub_cfg, kernel_basename, new_initrd_name, title, entry_id = sys.argv[1:6]
text = open(grub_cfg).read()

def find_block(start):
    brace_pos = text.find('{', start)
    if brace_pos == -1:
        return None
    depth = 0
    i = brace_pos
    while i < len(text):
        c = text[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
        i += 1
    return None

best = None
for m in re.finditer(r'^menuentry\b', text, re.MULTILINE):
    block = find_block(m.start())
    if block and kernel_basename in block and re.search(r'^\s*initrd\b', block, re.MULTILINE):
        best = block
        break

if best is None:
    sys.stderr.write("NO_MATCH\n")
    sys.exit(1)

lines = best.split('\n')
out = []
patched_title = False
patched_initrd = False
for line in lines:
    stripped = line.strip()
    if stripped.startswith('menuentry ') and not patched_title:
        esc_title = title.replace("'", "'\\''")
        line = re.sub(r"'([^']*)'", lambda m: "'" + esc_title + "'", line, count=1)
        line = re.sub(r"(\$menuentry_id_option\s+)'([^']*)'",
                       lambda m: m.group(1) + "'" + entry_id + "'", line)
        patched_title = True
    if stripped.startswith('initrd'):
        # Match this system's own convention: if the original entry points at
        # /boot/initrd.img-... (no separate /boot partition), the override
        # must use the same /boot/ prefix; if it points at /initrd.img-...
        # (grub root == the /boot partition itself), drop the prefix too.
        m2 = re.match(r'^\s*initrd\S*\s+(\S+)', line)
        orig_path = m2.group(1) if m2 else '/'
        orig_dir = orig_path.rsplit('/', 1)[0] if '/' in orig_path else ''
        new_path = f"{orig_dir}/{new_initrd_name}" if orig_dir else f"/{new_initrd_name}"
        indent = line[:len(line) - len(line.lstrip())]
        line = f"{indent}initrd\t{new_path}"
        patched_initrd = True
    out.append(line)

if not patched_initrd:
    sys.stderr.write("NO_INITRD_LINE\n")
    sys.exit(2)

print('\n'.join(out))
PYEOF
    ) || die 3 "could not find/patch a menuentry for $kernel_basename in $GRUB_CFG (run --dry-run -v to inspect, or pass --grub-cfg)"

    if [ "$DRY_RUN" -eq 1 ]; then
        printf '[dry-run] would write %s with this menuentry:\n' "$GRUB_FRAGMENT"
        printf '%s\n' "$patched" | sed 's/^/  /'
        return
    fi

    backup_file "$GRUB_CFG"
    {
        printf '#!/bin/sh\n'
        printf 'exec tail -n +3 "$0"\n'
        printf '# Managed by build_cedt_override_initrd.sh (entry-id: %s). Regenerated on every run -- do not hand-edit.\n' "$ENTRY_ID"
        printf '%s\n' "$patched"
    } > "$GRUB_FRAGMENT"
    chmod +x "$GRUB_FRAGMENT"
    log "wrote $GRUB_FRAGMENT"

    regen_grub_cfg
    if ! grep -q "$ENTRY_ID" "$GRUB_CFG"; then
        die 6 "regenerated $GRUB_CFG does not contain entry id $ENTRY_ID; check $GRUB_FRAGMENT and re-run manually"
    fi
    log "confirmed entry '$ENTRY_ID' present in $GRUB_CFG"
}

cleanup_legacy_grub2() {
    local found=0
    if [ -f "$GRUB_FRAGMENT" ] && grep -q "entry-id: $ENTRY_ID" "$GRUB_FRAGMENT" 2>/dev/null; then
        found=1
        act rm -f "$GRUB_FRAGMENT"
    fi
    local out="$BOOT_DIR/$OUT_INITRD_NAME"
    if [ -f "$out" ]; then
        found=1
        act rm -f "$out"
    fi
    [ "$found" -eq 1 ] || die 8 "nothing to clean up for entry-id $ENTRY_ID"
    backup_file "$GRUB_CFG"
    regen_grub_cfg
    log "removed entry '$ENTRY_ID' and $out"
}

# --- BLS / grubby ---
install_bls_grubby() {
    local out="$BOOT_DIR/$OUT_INITRD_NAME"
    act grubby --add-kernel="$KERNEL_IMAGE" --initrd="$out" --title="$ENTRY_TITLE" --copy-default
    log "added grubby entry titled '$ENTRY_TITLE'"
}

find_grubby_index() {
    grubby --info=ALL 2>/dev/null | awk -v t="title=\"${ENTRY_TITLE}\"" '
        /^index=/ { idx=$0; sub("index=", "", idx) }
        $0 == t { print idx; exit }
    '
}

cleanup_bls_grubby() {
    local idx
    idx=$(find_grubby_index)
    [ -n "$idx" ] || die 8 "no grubby entry titled '$ENTRY_TITLE' found"
    act grubby --remove-kernel="index=$idx"
    local out="$BOOT_DIR/$OUT_INITRD_NAME"
    [ -f "$out" ] && act rm -f "$out"
    log "removed grubby entry index=$idx and $out"
}

print_next_steps() {
    cat <<EOF

Done. Default boot entry is UNCHANGED. Next steps:
  1. Reboot and select "$ENTRY_TITLE" from the boot menu to apply the override.
  2. After booting into it, verify with:
       dmesg | grep -i "ACPI:.*Table Upgraded\\|CEDT"
       cat /proc/iomem | grep -i "CXL Window"
       dmesg -w | grep -i "AER\\|Unsupported Request\\|Completion Timeout\\|mce:\\|cxl.*ras\\|cxl.*poison"
     Note: a region being created is NOT sufficient proof the data path works --
     actually write a pattern to /dev/daxX.Y and read it back while watching the
     dmesg filter above for the whole duration.

To revert:
  $PROG --cleanup --entry-id "$ENTRY_ID" --entry-title "$ENTRY_TITLE" --boot-dir "$BOOT_DIR" --out-initrd-name "$OUT_INITRD_NAME"
EOF
}

main() {
    if [ "$DO_CLEANUP" -eq 1 ]; then
        require_root
        detect_bootloader
        case "$BOOTLOADER" in
            legacy_grub2) cleanup_legacy_grub2 ;;
            bls_grubby)   cleanup_bls_grubby ;;
            systemd_boot) die 3 "systemd-boot cleanup not implemented; remove the /boot/loader/entries/*.conf entry manually" ;;
        esac
        exit 0
    fi

    require_root
    validate_cedt_table
    detect_kernel_initrd
    detect_bootloader

    case "$BOOTLOADER" in
        systemd_boot)
            die 3 "systemd-boot detected; this script only supports GRUB2 so far. Manually add a /boot/loader/entries/*.conf entry pointing initrd at the generated override initrd."
            ;;
    esac

    if [ "$ASSUME_YES" -ne 1 ] && [ "$DRY_RUN" -ne 1 ]; then
        printf 'About to write %s and install a GRUB entry "%s". Continue? [y/N] ' "$BOOT_DIR/$OUT_INITRD_NAME" "$ENTRY_TITLE"
        read -r reply
        case "$reply" in y|Y|yes|YES) ;; *) die 1 "aborted" ;; esac
    fi

    build_override_initrd

    case "$BOOTLOADER" in
        legacy_grub2) install_legacy_grub2 ;;
        bls_grubby)   install_bls_grubby ;;
    esac

    [ "$DRY_RUN" -eq 1 ] || print_next_steps
}

main
