#!/usr/bin/env python3
"""Dump PCIe RAS DES event counters from the host (no UART / VU).

RAS DES event counters live in the PCIe RAS DES extended capability
(VSEC ID 0x2). Host software reaches the same EVENT_COUNTER_CONTROL /
EVENT_COUNTER_DATA registers the firmware VU `pcieevt` uses.

Access methods:
  config  (default)  pread/pwrite /sys/bus/pci/devices/<bdf>/config
  pio                mmap BAR0 resource0 and use the 0x8000 indirect window
                     to SoC DBI (PORT0=0x24512000, PORT1=0x245b0000)

Requires root. Linux only.

Enable and dump are separate runs. Enable first, then dump manually.

Examples:
    Step1: here is about enable functions' cmd 
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --enable all
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --disable all
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --enable group --group 0
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --enable lane --lane 1
    
    Step2:here is about dump functions' cmd
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0
    sudo python3 pine_pcie_event_dump.py --bdf 0000:65:00.0 --nonzero
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --lane 1
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --dumpall
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 -dumpall
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --lane all --nonzero
    this one is all select which port
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --access pio --port 0
    sudo python3 pine_pcie_event_dump.py --bdf 65:00.0 --clear
"""

from __future__ import print_function

import argparse
import mmap
import os
import re
import struct
import sys

PCI_DEVICES = "/sys/bus/pci/devices"
BDF_RE = re.compile(r"^[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-7]$")

RAS_DES_EXT_CAP_ID = 0x000B
RAS_DES_VSEC_ID = 0x0002
RAS_DES_CAP_OFF_DEFAULT = 0x250
RAS_DES_CAP_OFF_DEFAULT_PORT1 = 0x208
EVENT_CNT_CTRL_REL = 0x08
EVENT_CNT_DATA_REL = 0x0C

EVENT_GROUP_SHIFT = 24
EVENT_NUM_SHIFT = 16
EVENT_LANE_SHIFT = 8
EVENT_ENABLE_SHIFT = 2
EVENT_STATUS_BIT = 7
EVENT_LANE_MAX = 15
EVENT_LANE_COUNT = 8

# EVENT_COUNTER_ENABLE [4:2]. The field reads back as 0; any code other than
# the four below is defined as "no change". There is no per-lane or per-group
# code, so those are done by writing PER_EVENT_ON once per counter.
EVENT_EN_NO_CHANGE = 0x0
EVENT_EN_PER_EVENT_OFF = 0x1
EVENT_EN_PER_EVENT_ON = 0x3
EVENT_EN_ALL_OFF = 0x5
EVENT_EN_ALL_ON = 0x7
EVENT_NO_EN_CHANGE = EVENT_EN_NO_CHANGE

# EVENT_COUNTER_CLEAR [1:0]. Reads back as 0; other codes are reserved.
EVENT_NO_CLEAR = 0x0
EVENT_PER_CLEAR = 0x1
EVENT_ALL_CLEAR = 0x3

PORT0_DBI_CFG = 0x24512000        
PORT1_DBI_CFG = 0x245b0000
PIO_ADDR_L = 0x8000
PIO_ADDR_H = 0x8004
PIO_WR = 0x8008
PIO_RD = 0x800C
PIO_MAP_MIN = 0x8010

# (group, event, per_lane, name) — same table as vu_handler_pcieeventdump
EVENT_ENTRIES = (
    (0, 0x00, True, "EBUF Overflow"),
    (0, 0x01, True, "EBUF Under-run"),
    (0, 0x02, True, "Decode Error"),
    (0, 0x03, True, "Running Disparity Error"),
    (0, 0x04, True, "SKP OS Parity Error"),
    (0, 0x05, True, "SYNC Header Error"),
    (0, 0x06, True, "RX Valid De-assertion"),
    (0, 0x07, True, "CTL SKP OS Parity Error"),
    (0, 0x08, True, "1st Retimer Parity Error"),
    (0, 0x09, True, "2nd Retimer Parity Error"),
    (0, 0x0A, True, "Margin CRC and Parity Error"),
    (1, 0x05, False, "Detect EI Infer"),
    (1, 0x06, False, "Receiver Error"),
    (1, 0x07, False, "RX Recovery Request"),
    (1, 0x08, False, "N_FTS Timeout"),
    (1, 0x09, False, "Framing Error"),
    (1, 0x0A, False, "Deskew Error"),
    (1, 0x0B, False, "Unexpected EIOS"),
    (1, 0x0C, False, "Framing Error in L0"),
    (1, 0x0D, False, "Deskew Uncompleted Error"),
    (2, 0x00, False, "Bad TLP"),
    (2, 0x01, False, "LCRC Error"),
    (2, 0x02, False, "Bad DLLP"),
    (2, 0x03, False, "Replay Number Rollover"),
    (2, 0x04, False, "Replay Timeout"),
    (2, 0x05, False, "RX NAK DLLP"),
    (2, 0x06, False, "TX NAK DLLP"),
    (2, 0x07, False, "Retry TLP"),
    (3, 0x00, False, "FC Timeout"),
    (3, 0x01, False, "Poisoned TLP"),
    (3, 0x02, False, "ECRC Error"),
    (3, 0x03, False, "Unsupported Request"),
    (3, 0x04, False, "Completer Abort"),
    (3, 0x05, False, "Completion Timeout"),
    (4, 0x00, True, "EBUF SKP Add"),
    (4, 0x01, True, "EBUF SKP Delete"),
    (5, 0x00, False, "L0 to Recovery Entry"),
    (5, 0x01, False, "L1 to Recovery Entry"),
    (5, 0x02, False, "TX L0s Entry"),
    (5, 0x03, False, "RX L0s Entry"),
    (5, 0x04, False, "ASPM L1 Reject"),
    (5, 0x05, False, "L1 Entry"),
    (5, 0x06, False, "L1 CPM"),
    (5, 0x07, False, "L1.1 Entry"),
    (5, 0x08, False, "L1.2 Entry"),
    (5, 0x09, False, "L1 Short Duration"),
    (5, 0x0A, False, "L1.2 Abort"),
    (5, 0x0B, False, "L2 Entry"),
    (5, 0x0C, False, "Speed Change"),
    (5, 0x0D, False, "Link Width Change"),
    (6, 0x00, False, "TX ACK DLLP"),
    (6, 0x01, False, "TX Update FC DLLP"),
    (6, 0x02, False, "RX ACK DLLP"),
    (6, 0x03, False, "RX Update FC DLLP"),
    (6, 0x04, False, "RX Nullified TLP"),
    (6, 0x05, False, "TX Nullified TLP"),
    (6, 0x06, False, "RX Duplicate TLP"),
    (7, 0x00, False, "TX Memory Write"),
    (7, 0x01, False, "TX Memory Read"),
    (7, 0x02, False, "TX Configuration Write"),
    (7, 0x03, False, "TX Configuration Read"),
    (7, 0x04, False, "TX IO Write"),
    (7, 0x05, False, "TX IO Read"),
    (7, 0x06, False, "TX Completion without Data"),
    (7, 0x07, False, "TX Completion with Data"),
    (7, 0x08, False, "TX Message TLP"),
    (7, 0x09, False, "TX Atomic"),
    (7, 0x0A, False, "TX TLP with Prefix"),
    (7, 0x0B, False, "RX Memory Write"),
    (7, 0x0C, False, "RX Memory Read"),
    (7, 0x0D, False, "RX Configuration Write"),
    (7, 0x0E, False, "RX Configuration Read"),
    (7, 0x0F, False, "RX IO Write"),
    (7, 0x10, False, "RX IO Read"),
    (7, 0x11, False, "RX Completion without Data"),
    (7, 0x12, False, "RX Completion with Data"),
    (7, 0x13, False, "RX Message TLP"),
    (7, 0x14, False, "RX Atomic"),
    (7, 0x15, False, "RX TLP with Prefix"),
    (7, 0x16, False, "TX CCIX TLP"),
    (7, 0x17, False, "RX CCIX TLP"),
    (7, 0x18, False, "TX Deferrable Memory Write TLP"),
    (7, 0x19, False, "RX Deferrable Memory Write TLP"),
    (8, 0x00, False, "ALMP Replication Error"),
    (8, 0x01, False, "Unexpected ALMP Error"),
    (8, 0x02, False, "NUM_RETRY Reached MAX_NUM_RETRY"),
)


class ScriptError(Exception):
    """Fatal condition that should abort the run."""


def log_line(text):
    print(text)
    sys.stdout.flush()


def normalize_bdf(bdf_arg):
    bdf = bdf_arg if BDF_RE.match(bdf_arg) else "0000:" + bdf_arg
    if not BDF_RE.match(bdf):
        raise ScriptError("invalid BDF: %s" % bdf_arg)
    if not os.path.isdir(os.path.join(PCI_DEVICES, bdf)):
        raise ScriptError("no such PCI device: %s" % bdf)
    return bdf


def event_ctrl_word(group, event, lane, en, clr):
    return ((group << EVENT_GROUP_SHIFT) |
            (event << EVENT_NUM_SHIFT) |
            (lane << EVENT_LANE_SHIFT) |
            (en << EVENT_ENABLE_SHIFT) |
            clr)


class ConfigSpace(object):
    def __init__(self, bdf):
        path = os.path.join(PCI_DEVICES, bdf, "config")
        self._fd = os.open(path, os.O_RDWR)
        self._size = os.fstat(self._fd).st_size
        if self._size < 0x260:
            os.close(self._fd)
            self._fd = None
            raise ScriptError("%s is only %d bytes; need extended config" %
                              (path, self._size))

    def read32(self, off):
        raw = os.pread(self._fd, 4, off)
        if len(raw) != 4:
            raise ScriptError("config read 0x%x failed" % off)
        return struct.unpack("<I", raw)[0]

    def write32(self, off, value):
        n = os.pwrite(self._fd, struct.pack("<I", value & 0xFFFFFFFF), off)
        if n != 4:
            raise ScriptError("config write 0x%x failed" % off)

    def close(self):
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None


class Bar2Pio(object):
    def __init__(self, bdf, port):
        path = os.path.join(PCI_DEVICES, bdf, "resource2")
        if not os.path.exists(path):
            raise ScriptError("missing %s" % path)
        self._fd = os.open(path, os.O_RDWR | os.O_SYNC)
        size = os.fstat(self._fd).st_size
        if size < PIO_MAP_MIN:
            os.close(self._fd)
            self._fd = None
            raise ScriptError("%s size 0x%x too small for PIO window" % (path, size))
        self._mm = mmap.mmap(self._fd, size, mmap.MAP_SHARED,
                             mmap.PROT_READ | mmap.PROT_WRITE)
        self._dbi = PORT0_DBI_CFG if port == 0 else PORT1_DBI_CFG
        log_line("mmap %s size=0x%x dbi=0x%x" % (path, size, self._dbi))

    def _pio_set_addr(self, addr):
        struct.pack_into("<I", self._mm, PIO_ADDR_H, (addr >> 32) & 0xFFFFFFFF)
        struct.pack_into("<I", self._mm, PIO_ADDR_L, addr & 0xFFFFFFFF)

    def read32(self, off):
        self._pio_set_addr(self._dbi + off)
        return struct.unpack_from("<I", self._mm, PIO_RD)[0]

    def write32(self, off, value):
        self._pio_set_addr(self._dbi + off)
        struct.pack_into("<I", self._mm, PIO_WR, value & 0xFFFFFFFF)

    def close(self):
        if self._mm is not None:
            self._mm.close()
            self._mm = None
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None


RAS_DES_CAP_OFF_BY_PORT = {
    0: RAS_DES_CAP_OFF_DEFAULT,
    1: RAS_DES_CAP_OFF_DEFAULT_PORT1,
}


def ras_des_cap_hint(port):
    try:
        return RAS_DES_CAP_OFF_BY_PORT[port]
    except KeyError:
        raise ScriptError("unsupported port %r; expected one of %s" %
                          (port, sorted(RAS_DES_CAP_OFF_BY_PORT)))


def _is_ras_des_cap(bus, off):
    hdr = bus.read32(off)
    if (hdr & 0xFFFF) != RAS_DES_EXT_CAP_ID:
        return False
    vsec = bus.read32(off + 4)
    return (vsec & 0xFFFF) == RAS_DES_VSEC_ID


def find_ras_des_cap(bus, port=0, hint=None):
    """Locate the RAS DES capability offset.

    The well-known offset for the given port (PORT0 -> RAS_DES_CAP_OFF_DEFAULT,
    PORT1 -> RAS_DES_CAP_OFF_DEFAULT_PORT1) is probed first; an explicit
    ``hint`` overrides that choice. If the probe fails, the extended capability
    list is walked from 0x100 as a fallback.
    """
    if hint is None:
        hint = ras_des_cap_hint(port)
    if _is_ras_des_cap(bus, hint):
        return hint
    log_line("RAS DES not at port%d default 0x%x; scanning capability list" %
             (port, hint))

    off = 0x100
    seen = set()
    while off and off < 0xFFF and off not in seen:
        seen.add(off)
        if _is_ras_des_cap(bus, off):
            return off
        off = (bus.read32(off) >> 20) & 0xFFF
    raise ScriptError("RAS DES capability (VSEC ID 0x2) not found")


class RasDes(object):
    def __init__(self, bus, cap_off):
        self.bus = bus
        self.cap_off = cap_off
        self.ctrl_off = cap_off + EVENT_CNT_CTRL_REL
        self.data_off = cap_off + EVENT_CNT_DATA_REL

    def write_ctrl(self, group, event, lane, en, clr):
        self.bus.write32(self.ctrl_off,
                         event_ctrl_word(group, event, lane, en, clr))

    def read_data(self):
        return self.bus.read32(self.data_off)

    def read_ctrl(self):
        return self.bus.read32(self.ctrl_off)

    def selected_enabled(self):
        """EVENT_COUNTER_STATUS [7] of the counter last selected."""
        return (self.read_ctrl() >> EVENT_STATUS_BIT) & 1

    def _set_one(self, group, event, lane, on):
        """Per-event on/off for the counter EVENT_SELECT + LANE_SELECT point at."""
        code = EVENT_EN_PER_EVENT_ON if on else EVENT_EN_PER_EVENT_OFF
        self.write_ctrl(group, event, lane, code, EVENT_NO_CLEAR)
        enabled = self.selected_enabled()
        if enabled != (1 if on else 0):
            log_line("warning: grp%u event0x%02x lane%u status=%u after %s" % (
                group, event, lane, enabled, "enable" if on else "disable"))
        return enabled

    def enable_all(self, on=True, lane_count=EVENT_LANE_COUNT):
        """Enable or disable every counter in all 9 groups on 8 lanes.

        ALL_ON ignores lane select, so this walks each group and writes
        per-event on/off. Per-lane events (groups 0 and 4) are programmed on
        lane 0..lane_count-1; events that are not per-lane are programmed once.
        """
        groups = event_group_ids()
        if len(groups) != 9:
            raise ScriptError("expected 9 event groups, table has %d: %s" %
                              (len(groups), groups))
        if lane_count < 1 or lane_count > EVENT_LANE_MAX + 1:
            raise ScriptError("lane_count must be 1..%d" % (EVENT_LANE_MAX + 1))
        count = 0
        for group in groups:
            count += self.enable_group(group, on, lane_max=lane_count - 1)
        return count

    def disable_all(self, lane_count=EVENT_LANE_COUNT):
        """EVENT_EN_ALL_OFF: disable every counter in all 9 groups on 8 lanes.

        Same coverage as enable_all. The hardware ALL_OFF code ignores lane
        select, so each counter is written with PER_EVENT_OFF and checked
        against EVENT_COUNTER_STATUS.
        """
        return self.enable_all(on=False, lane_count=lane_count)

    def enable_group(self, group, on=True, lane_max=EVENT_LANE_MAX):
        """Enable or disable every counter in one group.

        Per-lane events (groups 0 and 4) are programmed on lane 0..lane_max,
        because a group enable has to cover each lane separately.
        """
        entries = [e for e in EVENT_ENTRIES if e[0] == group]
        if not entries:
            raise ScriptError("unknown event group %d; valid groups: %s" %
                              (group, event_group_ids()))
        count = 0
        for grp, event, per_lane, _name in entries:
            lanes = range(lane_max + 1) if per_lane else (0,)
            for lane in lanes:
                self._set_one(grp, event, lane, on)
                count += 1
        return count

    def enable_lane(self, lane, on=True):
        """Enable or disable every per-lane counter (groups 0 and 4) on one lane.

        Counters that are not per-lane have no lane select, so they are left
        untouched; use enable_group() or enable_all() for those.
        """
        count = 0
        for grp, event, per_lane, _name in EVENT_ENTRIES:
            if not per_lane:
                continue
            self._set_one(grp, event, lane, on)
            count += 1
        if count == 0:
            raise ScriptError("no per-lane event counters in the table")
        return count


def event_group_ids():
    groups = []
    for group, _event, _per_lane, _name in EVENT_ENTRIES:
        if group not in groups:
            groups.append(group)
    return groups


def apply_enable(ras, mode, group, lane, on):
    action = "enabled" if on else "disabled"
    if mode == "all":
        count = ras.disable_all() if not on else ras.enable_all()
        log_line("%s all 9 groups on lane 0..%d (%d counters)" % (
            action, EVENT_LANE_COUNT - 1, count))
        return
    if mode == "group":
        if group is None:
            raise ScriptError("--enable group requires --group (%s)" %
                              ", ".join(str(g) for g in event_group_ids()))
        count = ras.enable_group(group, on)
        log_line("%s group %d (%d counters, per-lane events on lane 0..%d)" % (
            action, group, count, EVENT_LANE_MAX))
        return
    if mode == "lane":
        count = ras.enable_lane(lane, on)
        log_line("%s lane %d (%d per-lane counters in groups 0 and 4)" % (
            action, lane, count))
        return
    raise ScriptError("unknown enable mode %r" % mode)


def dump_events(ras, lanes, clear, nonzero):
    """Dump counters.

    ``lanes`` is the list of lanes to read for per-lane events (groups 0 and 4).
    Events that are not per-lane are read once; repeating them per lane would
    print the same counter EVENT_LANE_COUNT times.
    """
    if clear:
        ras.write_ctrl(0, 0, 0, EVENT_NO_EN_CHANGE, EVENT_ALL_CLEAR)
        log_line("cleared all event counters (ALL_CLEAR)")

    show_lane = len(lanes) > 1
    printed = 0
    for group, event, per_lane, name in EVENT_ENTRIES:
        if per_lane:
            continue
        printed += _dump_one(ras, group, event, 0, name, None, nonzero)
    for lane in lanes:
        for group, event, per_lane, name in EVENT_ENTRIES:
            if not per_lane:
                continue
            printed += _dump_one(ras, group, event, lane, name,
                                 lane if show_lane else None, nonzero)
    if nonzero and printed == 0:
        log_line("all selected counters are 0")


def _dump_one(ras, group, event, lane, name, show_lane, nonzero):
    ras.write_ctrl(group, event, lane, EVENT_NO_EN_CHANGE, EVENT_NO_CLEAR)
    value = ras.read_data()
    enabled = ras.selected_enabled()
    if nonzero and value == 0:
        return 0
    prefix = "lane%-2u " % show_lane if show_lane is not None else ""
    log_line("%sgrp%u event0x%02x %-32s: 0x%x%s" % (
        prefix, group, event, name, value, "" if enabled else "  [disabled]"))
    return 1


def parse_lane(value):
    if value.lower() == "all":
        return "all"
    try:
        return int(value, 10)
    except ValueError:
        raise argparse.ArgumentTypeError("lane must be 0..%d or all" % EVENT_LANE_MAX)


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Dump PCIe RAS DES event counters via config space or BAR0 PIO")
    parser.add_argument("--bdf", required=True, help="PCI BDF, e.g. 65:00.0")
    parser.add_argument("--lane", type=parse_lane, default=None,
                        help="lane for per-lane groups 0 and 4 "
                             "(default 0). 'all' reads lane 0..%d" %
                             (EVENT_LANE_COUNT - 1))
    parser.add_argument("--dumpall", "-dumpall", action="store_true",
                        help="dump every counter on lane 0..%d, including zeros" %
                             (EVENT_LANE_COUNT - 1))
    parser.add_argument("--access", choices=("config", "pio"), default="config",
                        help="config space (default) or BAR2 PIO window")
    parser.add_argument("--port", type=int, default=0, choices=(0, 1),
                        help="controller port (default 0); selects the DBI base "
                             "for --access pio and the RAS DES capability "
                             "offset to probe first (port0 0x%x, port1 0x%x)" %
                             (RAS_DES_CAP_OFF_DEFAULT, RAS_DES_CAP_OFF_DEFAULT_PORT1))
    parser.add_argument("--clear", action="store_true",
                        help="ALL_CLEAR before dump")
    parser.add_argument("--nonzero", action="store_true",
                        help="print only counters with a non-zero value")
    parser.add_argument("--enable", choices=("all", "group", "lane"),
                        help="enable only; does not dump. Run again without "
                             "--enable to dump. "
                             "all: all 9 groups, per-lane events on lane 0..%d; "
                             "group: every event in --group "
                             "(per-lane events on lane 0..%d); "
                             "lane: every per-lane event on --lane. "
                             "Counters are disabled by default" %
                             (EVENT_LANE_COUNT - 1, EVENT_LANE_MAX))
    parser.add_argument("--group", type=int,
                        help="event group for --enable group (valid: %s)" %
                             ", ".join(str(g) for g in event_group_ids()))
    parser.add_argument("--disable", choices=("all",),
                        help="disable only; does not dump. "
                             "all: EVENT_EN_ALL_OFF for all 9 groups, "
                             "per-lane events on lane 0..%d" %
                             (EVENT_LANE_COUNT - 1))
    parser.add_argument("--off", action="store_true",
                        help="with --enable, write PER_EVENT_OFF instead of PER_EVENT_ON")
    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)
    if os.geteuid() != 0:
        raise ScriptError("must run as root")
    if args.lane not in (None, "all"):
        if args.lane < 0 or args.lane > EVENT_LANE_MAX:
            raise ScriptError("lane must be 0..%d or all" % EVENT_LANE_MAX)
    if args.off and not args.enable:
        raise ScriptError("--off requires --enable")
    if args.enable and args.disable:
        raise ScriptError("--enable and --disable cannot be used together")
    if (args.enable or args.disable) and (args.clear or args.nonzero or args.dumpall):
        raise ScriptError("--enable/--disable and dump options "
                          "(--clear/--nonzero/--dumpall) cannot be used together")
    if args.enable == "lane" and args.lane == "all":
        raise ScriptError("--enable lane needs one lane number, not all")
    if args.dumpall and args.nonzero:
        raise ScriptError("--dumpall prints every counter; omit --nonzero")
    if args.dumpall and args.lane not in (None, "all"):
        raise ScriptError("--dumpall already covers lane 0..%d; omit --lane" %
                          (EVENT_LANE_COUNT - 1))

    bdf = normalize_bdf(args.bdf)
    if args.access == "config":
        bus = ConfigSpace(bdf)
    else:
        bus = Bar2Pio(bdf, args.port)
    try:
        cap_off = find_ras_des_cap(bus, args.port)
    except ScriptError:
        bus.close()
        raise
    log_line("device %s access=%s port=%d ras_des=0x%x" %
             (bdf, args.access, args.port, cap_off))

    try:
        ras = RasDes(bus, cap_off)
        if args.disable:
            count = ras.disable_all()
            log_line("disabled all 9 groups on lane 0..%d (%d counters)" % (
                EVENT_LANE_COUNT - 1, count))
            log_line("disable done; run again without --disable to dump")
        elif args.enable:
            enable_lane = 0 if args.lane is None else args.lane
            apply_enable(ras, args.enable, args.group, enable_lane, not args.off)
            log_line("enable done; run again without --enable to dump")
        else:
            if args.dumpall or args.lane == "all":
                lanes = list(range(EVENT_LANE_COUNT))
                lane_text = "0..%d" % (EVENT_LANE_COUNT - 1)
            else:
                lanes = [0 if args.lane is None else args.lane]
                lane_text = str(lanes[0])
            log_line("PCIe event counters: bdf=%s lane=%s" % (bdf, lane_text))
            dump_events(ras, lanes, args.clear, args.nonzero and not args.dumpall)
    finally:
        bus.close()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except ScriptError as exc:
        sys.stderr.write("error: %s\n" % exc)
        sys.exit(1)
