import os
import sys
import struct
import mmap
import argparse

# PCIe Header Type 0/1的字段定义
PCI_HEADER_FIELDS = {
    "VendorID": (0x00, "H"),
    "DeviceID": (0x02, "H"),
    "Command": (0x04, "H"),
    "Status": (0x06, "H"),
    "RevisionID": (0x08, "B"),
    "ClassCode": (0x09, "B"),
    "Subclass": (0x0A, "B"),
    "ProgIf": (0x0B, "B"),
    "CacheLineSize": (0x0C, "B"),
    "LatencyTimer": (0x0D, "B"),
    "HeaderType": (0x0E, "B"),
    "BIST": (0x0F, "B"),
    "BAR0": (0x10, "I"),
    "BAR1": (0x14, "I"),
    "BAR2": (0x18, "I"),
    "BAR3": (0x1C, "I"),
    "BAR4": (0x20, "I"),
    "BAR5": (0x24, "I"),
    "CardbusCIS": (0x28, "I"),
    "SubsystemVendorID": (0x2C, "H"),
    "SubsystemID": (0x2E, "H"),
    "ExpansionROMBaseAddress": (0x30, "I"),
    "CapabilitiesPtr": (0x34, "B"),
    "InterruptLine": (0x3C, "B"),
    "InterruptPin": (0x3D, "B"),
    "MinGrant": (0x3E, "B"),
    "MaxLatency": (0x3F, "B")
}

def parse_pci_header(data):
    if len(data) < 256:
        print("错误: 数据不足256字节，无法解析PCIe Header")
        return
    
    header = {}
    for field, (offset, fmt) in PCI_HEADER_FIELDS.items():
        value = struct.unpack_from(f"<{fmt}", data, offset)[0]
        header[field] = value
    
    # 打印类似lspci -vvv的输出
    print("\nPCIe Header解析结果:")
    print(f"00:00.0 PCI bridge: VendorID={header['VendorID']:04x} DeviceID={header['DeviceID']:04x}")
    print(f"\tCommand: {header['Command']:04x}")
    print(f"\tStatus: {header['Status']:04x}")
    print(f"\tClass: {header['ClassCode']:02x}{header['Subclass']:02x}{header['ProgIf']:02x}")
    print(f"\tHeader Type: {header['HeaderType']:02x}")
    print(f"\tBAR0: {header['BAR0']:08x}")
    print(f"\tBAR1: {header['BAR1']:08x}")
    print(f"\tBAR2: {header['BAR2']:08x}")
    print(f"\tBAR3: {header['BAR3']:08x}")
    print(f"\tBAR4: {header['BAR4']:08x}")
    print(f"\tBAR5: {header['BAR5']:08x}")
    print(f"\tSubsystem: VendorID={header['SubsystemVendorID']:04x} DeviceID={header['SubsystemID']:04x}")
    print(f"\tInterrupt: Line={header['InterruptLine']:02x} Pin={header['InterruptPin']:02x}")
    print(f"\tCapabilities: Pointer={header['CapabilitiesPtr']:02x}")

def dump_memory(phys_addr, size, parse_pci=False):
    try:
        # 打开/dev/mem设备文件
        with open('/dev/mem', 'r+b') as f:
            # 使用mmap映射物理内存
            mem = mmap.mmap(f.fileno(), size, offset=phys_addr)
            # 读取数据
            data = mem.read(size)
            mem.close()
        
        # 先显示hexdump结果
        print("Hexdump结果:")
        offset = 0
        while offset < size:
            # 每行显示16字节
            line_data = data[offset:offset+16]
            # 打印偏移量
            print(f"{offset:04x}:", end=' ')
            # 打印16进制值
            for byte in line_data:
                print(f"{byte:02x}", end=' ')
            # 打印ASCII字符
            print(" ", end='')
            for byte in line_data:
                if 32 <= byte <= 126:
                    print(chr(byte), end='')
                else:
                    print('.', end='')
            print()
            offset += 16
        
        # 如果需要解析PCIe Header
        if parse_pci:
            parse_pci_header(data)
    
    except Exception as e:
        print(f"错误: 无法访问物理内存 - {str(e)}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Dump physical memory")
    parser.add_argument("phys_addr", help="Physical address in hex")
    parser.add_argument("size", type=int, help="Number of bytes to dump")
    parser.add_argument("-p", "--pci", action="store_true", help="Parse as PCIe header")
    args = parser.parse_args()
    
    try:
        # 解析输入的物理地址
        phys_addr = int(args.phys_addr, 16)
        
        # 检查地址和大小是否有效
        if phys_addr < 0 or args.size <= 0:
            raise ValueError
        
        # 调用dump函数
        dump_memory(phys_addr, args.size, args.pci)
    
    except ValueError:
        print("错误: 请输入有效的16进制地址和正整数字节数")
        sys.exit(1)

if __name__ == "__main__":
    main() 