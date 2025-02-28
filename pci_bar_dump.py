#!/usr/bin/env python3
import os
import sys
import argparse
import re
import subprocess

def normalize_slot(slot):
    """标准化PCI设备地址格式"""
    if not slot.startswith("0000:"):
        return f"0000:{slot}"
    return slot

def convert_size(size_str, unit):
    """增强的单位转换逻辑"""
    unit = unit.upper().replace('B', '')  # 统一处理K/KB的情况
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
    """改进的lspci解析逻辑"""
    try:
        output = subprocess.check_output(
            ['lspci', '-vvv', '-s', pci_slot],
            stderr=subprocess.STDOUT
        ).decode()
        
        bars = []
        pattern = re.compile(
            r"Memory at (\w+) \((.*?)\) \[size=(\d+)([KMG]?B?)\]"
        )
        
        bar_num = 0
        for line in output.split('\n'):
            if 'Memory at' in line:
                match = pattern.search(line)
                if match:
                    address = int(match.group(1), 16)
                    flags = match.group(2)
                    size_str = match.group(3)
                    unit = match.group(4)
                    
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
                    bar_num += 1
        return bars
    except Exception as e:
        print(f"错误: 无法解析lspci输出 - {str(e)}", file=sys.stderr)
        sys.exit(1)

def read_with_devmem(address, size, width=32, busybox_path="./busybox"):
    """使用busybox devmem读取内存"""
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
        print(f"读取0x{address:x}失败: {str(e)}")
        return None

def format_hex_dump(data, base_address, bytes_per_line=16):
    """格式化十六进制输出"""
    result = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i+bytes_per_line]
        addr = base_address + i
        hex_str = ' '.join(f"{b:02x}" for b in chunk)
        ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        result.append(f"{addr:016x}: {hex_str.ljust(47)}  {ascii_str}")
    return '\n'.join(result)

def main():
    parser = argparse.ArgumentParser(
        description="PCIe BAR空间解析工具（基于lspci和devmem）",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("slot", 
        help="PCI设备位置，支持以下格式：\n"
             " - 简短格式: 01:00.0\n"
             " - 完整格式: 0000:01:00.0")
    parser.add_argument("-b", "--bar", type=int,
        help="指定要dump的BAR编号")
    parser.add_argument("-o", "--offset", type=lambda x: int(x,0), default=0,
        help="指定起始偏移量（支持十六进制，如0x100）")
    parser.add_argument("-l", "--dump_len", type=lambda x: int(x,0), default=128,
        help="指定要dump的长度（字节数，支持十六进制，默认128)")
    parser.add_argument("--busybox", default="./busybox",
        help="指定busybox路径（默认当前目录）")

    args = parser.parse_args()

    # 标准化设备地址
    normalized_slot = normalize_slot(args.slot)
    
    # 验证busybox可用性
    if not os.path.exists(args.busybox):
        print(f"错误: 找不到busybox ({args.busybox})")
        sys.exit(1)

    # 获取BAR信息
    bars = get_bars_from_lspci(normalized_slot)
    
    if not bars:
        print("错误: 未检测到有效BAR")
        sys.exit(1)

    # 过滤目标BAR
    if args.bar is not None:
        bars = [b for b in bars if b['num'] == args.bar]
        if not bars:
            print(f"错误: 未找到BAR{args.bar}")
            sys.exit(1)

    # 处理每个BAR
    for bar in bars:
        # 验证偏移量有效性
        if args.offset < 0 or args.offset >= bar['size']:
            print(f"错误: BAR{bar['num']}的偏移量0x{args.offset:x}超出范围 (0-0x{bar['size']-1:x})")
            sys.exit(1)
            
        # 计算实际读取参数
        start_addr = bar['start'] + args.offset
        max_readable = bar['size'] - args.offset
        read_length = min(args.dump_len, max_readable)

        # 输出BAR信息
        print(f"\nPCI设备 {normalized_slot} BAR{bar['num']}信息:")
        print(f"  类型: {bar['type']}")
        print(f"  地址范围: 0x{bar['start']:016x}-0x{bar['start']+bar['size']:016x}")
        print(f"  本次读取范围: 0x{start_addr:016x}-0x{start_addr+read_length:016x}")

        # Dump内容
        data = read_with_devmem(start_addr, read_length, busybox_path=args.busybox)
        if data:
            print("\nHex dump:")
            print(format_hex_dump(data, start_addr))
        else:
            print("无法读取BAR内容")

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("错误：需要root权限运行 (使用sudo执行)", file=sys.stderr)
        sys.exit(1)
    try:
        main()
    except KeyboardInterrupt:
        print("\n操作已取消")
        sys.exit(130)
