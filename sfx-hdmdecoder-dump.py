#!/usr/bin/env python3
import os
import sys
import argparse
import re
import subprocess

def normalize_slot(slot):
    """Standardize PCI device address format"""
    if not slot.startswith("0000:"):
        return f"0000:{slot}"
    return slot

def convert_size(size_str, unit):
    """Enhanced unit conversion logic"""
    unit = unit.upper().replace('B', '')  # Unify K/KB cases
    size = int(size_str)
    if 'K' in unit:
        return size * 1024
    elif 'M' in unit:
        return size * 1024 * 1024
    elif 'G' in unit:
        return size * 1024 * 1024 * 1024
    else:
        return size

def get_bars_from_lspci(pci_slot):
    """Improved lspci parsing logic"""
    try:
        output = subprocess.check_output(
            ['lspci', '-vvv', '-s', pci_slot],
            stderr=subprocess.STDOUT
        ).decode()
        
        bars = []
        pattern = re.compile(
            r"Region (\d+): Memory at (\w+) \((.*?)\) \[size=(\d+)([KMG]?B?)\]"
        )
        
        for line in output.split('\n'):
            if 'Region' in line and 'Memory at' in line:
                match = pattern.search(line)
                if match:
                    bar_num = int(match.group(1))  # Get bar_num from Region number
                    address = int(match.group(2), 16)
                    flags = match.group(3)
                    size_str = match.group(4)
                    unit = match.group(5)
                    
                    size = convert_size(size_str, unit)
                    bar_type = "MEM"
                    prefetch = "Prefetch" if 'prefetchable' in flags else ""
                    is_64bit = '64-bit' in flags
                    
                    bars.append({
                        'num': bar_num,
                        'start': address,
                        'size': size,
                        'type': f"{bar_type} {prefetch}".strip(),
                        '64bit': is_64bit
                    })
        return bars
    except Exception as e:
        print(f"Error: Failed to parse lspci output - {str(e)}", file=sys.stderr)
        sys.exit(1)

def read_with_devmem(address, size, width=32, busybox_path="./busybox"):
    """Read memory using busybox devmem"""
    try:
        data = bytearray()
        remaining = size
        current_addr = address
        
        while remaining > 0:
            output = subprocess.check_output(
                [busybox_path, 'devmem', f'0x{current_addr:x}', str(width)],
                stderr=subprocess.STDOUT
            )
            value = int(output.decode().strip(), 16)
            bytes_to_read = min(remaining, width//8)
            data.extend(value.to_bytes(width//8, byteorder='little')[:bytes_to_read])
            current_addr += bytes_to_read
            remaining -= bytes_to_read
        return bytes(data)
    except Exception as e:
        print(f"Failed to read 0x{address:x}: {str(e)}")
        return None

def format_hex_dump(data, base_address, bytes_per_line=16):
    """Format hexadecimal output"""
    result = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i+bytes_per_line]
        addr = base_address + i
        hex_str = ' '.join(f"{b:02x}" for b in chunk)
        ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        result.append(f"{addr:016x}: {hex_str.ljust(47)}  {ascii_str}")
    return '\n'.join(result)

def analyze_dumped_data(data, dump_raw=False):
    if not dump_raw:
        return  # Skip if not dumping raw data
    if not data:
        print("No data to analyze")
        return
    
    non_zero_count = sum(1 for byte in data if byte != 0)
    total_bytes = len(data)
    non_zero_ratio = (non_zero_count / total_bytes) * 100 if total_bytes > 0 else 0
    
    print(f"\nAnalysis results:")
    print(f"  Total bytes:\t{total_bytes}")
    print(f"  Non-zero bytes:\t{non_zero_count}")
    print(f"  Non-zero byte ratio:\t{non_zero_ratio:.2f}%")
    if non_zero_count > 0:
        print(f"  Non-zero data detected:\t1")
    else:
        print(f"  Non-zero data detected:\t0")

def parse_cxl_header(data):
    """Parse CXL Capability Header and subsequent Cap Array Entries"""
    if len(data) < 4:
        print("Data insufficient, cannot parse CXL Header")
        return
    
    # Parse the first DW with offset
    header_dw = int.from_bytes(data[0:4], byteorder='little')
    print(f"CXL Capability Header (0x00):\t0x{header_dw:08x}")
    cxl_capability_id = header_dw & 0xFFFF  # Bits 15:0
    cxl_cap_version = (header_dw >> 16) & 0xF  # Bits 19:16
    cxl_cache_mem_version = (header_dw >> 20) & 0xF  # Bits 23:20
    array_size = (header_dw >> 24) & 0xFF  # Bits 31:24
    
    print(f"  CXL Capability ID (bits 15:0):\t0x{cxl_capability_id:04x}")
    print(f"  CXL Capability Version (bits 19:16):\t0x{cxl_cap_version:02x}")
    print(f"  CXL Cache/mem Version (bits 23:20):\t0x{cxl_cache_mem_version:02x}")
    print(f"  Array Size (bits 31:24):\t0x{array_size:02x}")
    
    # Cap ID mapping
    cap_id_mapping = {
        2: "CXL RAS Cap",
        4: "CXL Link Cap",
        5: "CXL HDM Decoder"
    }
    
    # Parse subsequent Entries
    for i in range(array_size):
        entry_offset = 4 + (i * 4)
        if entry_offset + 4 > len(data):
            print(f"Warning: Data insufficient, skipping Entry {i+1}")
            break
        entry_dw = int.from_bytes(data[entry_offset:entry_offset+4], byteorder='little')
        print(f"Entry {i+1} (0x{entry_offset:02x}):\t0x{entry_dw:08x}")
        
        entry_cxl_cap_id = entry_dw & 0xFFFF  # Bits 15:0
        entry_cxl_cap_version = (entry_dw >> 16) & 0xF  # Bits 19:16
        capability_pointer = (entry_dw >> 20) & 0xFFF  # Bits 31:20
        description = cap_id_mapping.get(entry_cxl_cap_id, "Unknown Cap ID")
        
        print(f"  CXL Capability ID (bits 15:0):\t0x{entry_cxl_cap_id:04x} ({description})")
        print(f"  CXL Capability Version (bits 19:16):\t0x{entry_cxl_cap_version:02x}")
        print(f"  Capability Pointer (bits 31:20):\t0x{capability_pointer:03x}")
        
        if entry_cxl_cap_id == 5:
            print(f"\nParsing HDM Decoder registers (based on Pointer 0x{capability_pointer:03x})")
            parse_hdm_decoder_registers(data, capability_pointer)

def parse_hdm_decoder_registers(data, pointer_offset):
    """Parse HDM Decoder registers"""
    # HDM Decoder Capability Register
    if pointer_offset + 4 > len(data):
        print("Warning: Data insufficient, cannot parse HDM Decoder Capability Register")
        return
    hdm_cap_dw = int.from_bytes(data[pointer_offset:pointer_offset+4], byteorder='little')
    print(f"HDM Decoder Capability Register (0x{pointer_offset:03x}):\t0x{hdm_cap_dw:08x}")
    
    num_decoders_raw = hdm_cap_dw & 0xF  # Bits 3:0
    num_decoders = 2 ** num_decoders_raw  # Calculate 2^n
    target_count = (hdm_cap_dw >> 4) & 0xF  # Bits 7:4
    a11to8_interleave = (hdm_cap_dw >> 8) & 0x1  # Bit 8
    a14to12_interleave = (hdm_cap_dw >> 9) & 0x1  # Bit 9
    poison_on_error = (hdm_cap_dw >> 10) & 0x1  # Bit 10
    support_3_6_12_way = (hdm_cap_dw >> 11) & 0x1  # Bit 11
    support_16_way = (hdm_cap_dw >> 12) & 0x1  # Bit 12
    uio_capable = (hdm_cap_dw >> 13) & 0x1  # Bit 13
    
    print(f"  Number of Decoders (bits 3:0):\t0x{num_decoders_raw:02x} (decoder_num: {num_decoders})")
    print(f"  Target Count (bits 7:4):\t0x{target_count:02x}")
    print(f"  A11to8 Interleave Capable (bit 8):\t{a11to8_interleave}")
    print(f"  A14to12 Interleave Capable (bit 9):\t{a14to12_interleave}")
    print(f"  Support Poison on Decoder Error (bit 10):\t{poison_on_error}")
    print(f"  Support 3/6/12 Way Interleave (bit 11):\t{support_3_6_12_way}")
    print(f"  16 Way Interleave Capable (bit 12):\t{support_16_way}")
    print(f"  UIO Capable (bit 13):\t{uio_capable}")
    
    # HDM Decoder Global Control Register
    hdm_control_offset = pointer_offset + 4
    if hdm_control_offset + 4 > len(data):
        print("Warning: Data insufficient, cannot parse HDM Decoder Global Control Register")
        return
    hdm_control_dw = int.from_bytes(data[hdm_control_offset:hdm_control_offset+4], byteorder='little')
    print(f"HDM Decoder Global Control Register (0x{hdm_control_offset:03x}):\t0x{hdm_control_dw:08x}")
    
    poison_enable = hdm_control_dw & 0x1  # Bit 0
    decoder_enable = (hdm_control_dw >> 1) & 0x1  # Bit 1
    
    print(f"  Poison ON Decoder Error Enable (bit 0):\t{poison_enable}")
    print(f"  HDM Decoder Enable (bit 1):\t{decoder_enable}")
    
    # Parse each Decoder
    for decoder_index in range(num_decoders):
        decoder_start_offset = pointer_offset + 0x10 + (0x20 * decoder_index)
        if decoder_start_offset + 20 > len(data):
            print(f"Warning: Data insufficient, skipping Decoder {decoder_index}")
            break
        
        print(f"\nDecoder {decoder_index} parsing:")
        
        # DW0: Memory Base Low
        dw0_dw = int.from_bytes(data[decoder_start_offset:decoder_start_offset+4], byteorder='little')
        print(f"  Base Low (0x{decoder_start_offset:03x}):\t0x{dw0_dw:08x}")
        
        # DW1: Memory Base High
        dw1_dw = int.from_bytes(data[decoder_start_offset+4:decoder_start_offset+8], byteorder='little')
        memory_base = int.from_bytes(data[decoder_start_offset:decoder_start_offset+8], byteorder='little')
        print(f"  Base High (0x{decoder_start_offset+4:03x}):\t0x{dw1_dw:08x}")
        print(f"  Base (u64):\t0x{memory_base:016x}")
        
        # DW2: Memory Size Low
        dw2_dw = int.from_bytes(data[decoder_start_offset+8:decoder_start_offset+12], byteorder='little')
        print(f"  Size Low (0x{decoder_start_offset+8:03x}):\t0x{dw2_dw:08x}")
        
        # DW3: Memory Size High
        dw3_dw = int.from_bytes(data[decoder_start_offset+12:decoder_start_offset+16], byteorder='little')
        memory_size = int.from_bytes(data[decoder_start_offset+8:decoder_start_offset+16], byteorder='little')
        print(f"  Size High (0x{decoder_start_offset+12:03x}):\t0x{dw3_dw:08x}")
        print(f"  Size (u64):\t0x{memory_size:016x}")
        
        # DW4: Decoder i Control
        dw4_dw = int.from_bytes(data[decoder_start_offset+16:decoder_start_offset+20], byteorder='little')
        print(f"  Decoder Control (0x{decoder_start_offset+16:03x}):\t0x{dw4_dw:08x}")
        
        ig = dw4_dw & 0xF  # Bits 3:0
        iw = (dw4_dw >> 4) & 0xF  # Bits 7:4
        lock_on_commit = (dw4_dw >> 8) & 0x1  # Bit 8
        commit = (dw4_dw >> 9) & 0x1  # Bit 9
        committed = (dw4_dw >> 10) & 0x1  # Bit 10
        error_not_committed = (dw4_dw >> 11) & 0x1  # Bit 11
        target_range_type = (dw4_dw >> 12) & 0x1  # Bit 12
        bi_permitted = (dw4_dw >> 13) & 0x1  # Bit 13
        uio_permitted = (dw4_dw >> 14) & 0x1  # Bit 14
        uig = (dw4_dw >> 16) & 0xF  # Bits 19:16
        uiw = (dw4_dw >> 20) & 0xF  # Bits 23:20
        isp = (dw4_dw >> 24) & 0xF  # Bits 27:24
        
        print(f"    bIC (bits 3:0):\t0x{ig:02x}")
        print(f"    bIW (bits 7:4):\t0x{iw:02x}")
        print(f"    Lock on Commit (bit 8):\t{lock_on_commit}")
        print(f"    Commit (bit 9):\t{commit}")
        print(f"    Committed (bit 10):\t{committed}")
        print(f"    Error Not Committed (bit 11):\t{error_not_committed}")
        print(f"    Target Range Type (bit 12):\t{target_range_type}")
        print(f"    BI Permitted (bit 13):\t{bi_permitted}")
        print(f"    UIO Permitted (bit 14):\t{uio_permitted}")
        print(f"    bUIG (bits 19:16):\t0x{uig:02x}")
        print(f"    bUIW (bits 23:20):\t0x{uiw:02x}")
        print(f"    bISP (bits 27:24):\t0x{isp:02x}")

def main():
    parser = argparse.ArgumentParser(
        description="PCIe BAR space parsing tool (based on lspci and devmem)",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("slot", 
        help="PCI device location, formats:\n"
             " - Short: 01:00.0\n"
             " - Full: 0000:01:00.0")
    parser.add_argument("-b", "--bar", type=int, default=0,
        help="Specify BAR number to dump (default: 0)")
    parser.add_argument("-o", "--offset", type=lambda x: int(x,0), default=4096,
        help="Specify starting offset (hex supported, default: 0x1000)")
    parser.add_argument("-l", "--dump_len", type=lambda x: int(x,0), default=4096,
        help="Specify length to dump in bytes (hex supported, default: 0x1000)")
    parser.add_argument("--busybox", default="./busybox",
        help="Specify busybox path (default: current directory)")
    parser.add_argument("--dump-raw", action="store_true",
        help="Enable dumping of raw hex data and analysis")
    
    args = parser.parse_args()
    
    # Standardize device address
    normalized_slot = normalize_slot(args.slot)
    
    # Verify busybox availability
    if not os.path.exists(args.busybox):
        print(f"Error: Busybox not found ({args.busybox})")
        sys.exit(1)
    
    # Get BAR information
    bars = get_bars_from_lspci(normalized_slot)
    
    if not bars:
        print("Error: No valid BAR detected")
        sys.exit(1)
    
    # Filter target BAR
    bars = [b for b in bars if b['num'] == args.bar]
    if not bars:
        print(f"Error: BAR{args.bar} not found")
        sys.exit(1)
    
    for bar in bars:
        # Verify offset validity
        if args.offset < 0 or args.offset >= bar['size']:
            print(f"Error: BAR{bar['num']} offset 0x{args.offset:x} out of range (0-0x{bar['size']-1:x})")
            sys.exit(1)
        
        # Calculate actual read parameters
        start_addr = bar['start'] + args.offset
        max_readable = bar['size'] - args.offset
        read_length = min(args.dump_len, max_readable)
        
        # Output BAR information
        print(f"\nPCI device {normalized_slot} BAR{bar['num']} info:")
        print(f"  Type: {bar['type']}")
        print(f"  Address range: 0x{bar['start']:016x}-0x{bar['start']+bar['size']:016x}")
        print(f"  Read range: 0x{start_addr:016x}-0x{start_addr+read_length:016x}")
        
        # Dump content
        data = read_with_devmem(start_addr, read_length, busybox_path=args.busybox)
        if data:
            if args.dump_raw:
                print("\nHex dump:")
                print(format_hex_dump(data, start_addr))
                analyze_dumped_data(data, dump_raw=args.dump_raw)
            parse_cxl_header(data)
        else:
            print("Cannot read BAR content")

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("Error: Requires root privileges to run (use sudo)")
        sys.exit(1)
    try:
        main()
    except KeyboardInterrupt:
        print("\nOperation canceled")
        sys.exit(130)
