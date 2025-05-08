##!/usr/bin/python3

#need install the lib
# sudo pip3 install PySerial, pexpect_serial,  pyprind, xmodem


import serial
from pexpect_serial import SerialSpawn
from optparse import OptionParser
import xmodem
import os, sys, time
import logging
import pyprind

prompt = "SN@OPN:"
comport = "/dev/ttyUSB0"
global ret
ret  = "".encode()

def getc(size, timeout=1):
    return s.read(size)

def putc(data, timeout=1):
    global bar  # 使用全局变量 bar
    bar.update()
    return s.write(data)

def sendbin(cmd):
    strOut = ""
    print(f"send {cmd} command")
    s.write((cmd + "\r\n").encode())
    time.sleep(0.8)
    try:
        ret = s.read(32)
        strOut = (ret.decode())
    except:
        pass
    return strOut

def send_chunked_command(s, command, chunk_size=4, delay=0.1):
    """
    分块发送命令
    :param s: 串口对象
    :param command: 要发送的命令
    :param chunk_size: 每块的大小，默认为 4
    :param delay: 每块之间的延迟，默认为 0.1 秒
    """
    for i in range(0, len(command), chunk_size):
        s.write(command[i:i+chunk_size].encode())
        time.sleep(delay)
    s.write("\r\n".encode())  # 发送换行符
    time.sleep(2)  # 等待设备处理

if __name__ == '__main__':

    logging.basicConfig()
    parser = OptionParser(usage="python %prog [options]")
    parser.add_option("-f", dest="bin_path", help="path of BLx")
    parser.add_option("--cfg0", dest="cfg0_path", help="path of common.bin")
    parser.add_option("--cfg1", dest="cfg1_path", help="path of vendor3200.bin")
    parser.add_option("--cfg2", dest="cfg2_path", help="path of vendor4800.bin")
    parser.add_option("--sn", dest="sn", help="SN information")

    (opt, args) = parser.parse_args()

    if not (opt.bin_path and opt.cfg0_path and opt.cfg1_path and opt.cfg2_path):
        print("\nError: All file paths must be provided!\n")
        parser.print_help()
        sys.exit(-1)

    print("kill minicom")
    os.system("sudo killall -9 minicom")
    s = serial.Serial(comport,115200,timeout=1)

    # 持续监听串口，直到打印 "NOR task init done!"
    print("Listening to serial port...")
    while True:
        if s.in_waiting > 0:
            output = s.read(s.in_waiting).decode('latin-1')
            # 过滤并打印关键log
            for line in output.splitlines():
                if any(keyword in line for keyword in ["Ver ", "DDR Frequency", "Training has run successfully"]):
                    print(line)
            if "NOR task init done!" in output:
                print("NOR task init done!")
                break


    # 升级 BL2（如果提供了 -f 参数）
    if opt.bin_path:
        if not os.path.exists(opt.bin_path):
            print(f"\nError: File [ {opt.bin_path} ] not found !!!\n")
            sys.exit(-1)

        print("\n\033[1;36m=== Step 1: Updating BL2 ===\033[0m")
        statinfo_bin = os.stat(opt.bin_path)
        bar = pyprind.ProgBar(statinfo_bin.st_size/128+2)
        print("Start Update BL2")
        comOutput = sendbin("ndl2")
        while "NCCC" not in comOutput:
            comOutput = sendbin("ndl2")

        stream = open(opt.bin_path, 'rb')
        m = xmodem.XMODEM(getc, putc)
        m.send(stream)

        # 输出串口回送字符
        time.sleep(1)  # 等待传输完成
        output = ""
        while s.in_waiting > 0:
            output += s.read(s.in_waiting).decode('latin-1')
        print(output, end='')

        # 检测是否成功
        if "successfully executed vu: ndl2 command!" in output:
            print("\n\033[1;32mBL2 update: SUCCESS\033[0m")
        else:
            print("\n\033[1;31mBL2 update: FAILED\033[0m")
            sys.exit(-1)
    
    # 重启Board
    print("Restarting WS...")
    print("Turning off outlet 5")
    os.system('curl -u admin:1234 "http://10.33.104.109/outlet?5=OFF"')
    time.sleep(1)  # 等待1秒
    print("Turning on outlet 5")
    os.system('curl -u admin:1234 "http://10.33.104.109/outlet?5=ON"')
    print("WS restart completed")

    # 持续监听串口，直到打印 "NOR task init done!"
    print("Listening to serial port...")
    while True:
        if s.in_waiting > 0:
            output = s.read(s.in_waiting).decode('latin-1')
            # 过滤并打印关键log
            for line in output.splitlines():
                if any(keyword in line for keyword in ["Ver ", "DDR Frequency", "Training has run successfully"]):
                    print(line)
            if "NOR task init done!" in output:
                print("NOR task init done!")
                break

    # 升级 CFG 文件（如果提供了 -cfg0, -cfg1, -cfg2 参数）
    cfg_files = [
        (opt.cfg0_path, "common.bin"),
        (opt.cfg1_path, "vendor3200.bin"),
        (opt.cfg2_path, "vendor4800.bin")
    ]

    for cfg_path, cfg_name in cfg_files:
        if cfg_path:
            if not os.path.exists(cfg_path):
                print(f"\nError: File [ {cfg_path} ] not found !!!\n")
                sys.exit(-1)

            print(f"\n\033[1;36m=== Step 2: Updating {cfg_name} ===\033[0m")
            print("\033[1;33mSending CFG update command...\033[0m")
            comOutput = sendbin("ndlcfg")
            while "NCCC" not in comOutput:
                comOutput = sendbin("ndlcfg")

            statinfo_cfg = os.stat(cfg_path)
            bar = pyprind.ProgBar(statinfo_cfg.st_size/128+2)  # 初始化 bar
            stream = open(cfg_path, 'rb')
            m = xmodem.XMODEM(getc, putc)
            m.send(stream)

            # 输出串口回送字符
            time.sleep(5)  # 等待传输完成
            output = ""
            while s.in_waiting > 0:
                output += s.read(s.in_waiting).decode('latin-1')
            print(output, end='')

            # 检测是否成功
            if "updating success" in output:
                print(f"\n\033[1;32m{cfg_name} update: SUCCESS\033[0m")
            else:
                print(f"\n\033[1;31m{cfg_name} update: FAILED\033[0m")
                sys.exit(-1)

    # 更新 Mat 信息（如果提供了 --sn 参数）
    if opt.sn:
        print("\n\033[1;36m=== Step 3: Updating Mat Info ===\033[0m")
        print("\033[1;33mSending Mat update command...\033[0m")
        mat_cmd = f"writematheader {opt.sn} opn-1 MCM500 A1 ScaleFlux Cypress"
        send_chunked_command(s, mat_cmd)  # 使用分块发送函数

        # 输出串口回送字符
        output = ""
        while s.in_waiting > 0:
            output += s.read(s.in_waiting).decode('latin-1')
        print(output, end='')

        # 检测是否成功
        if "successfully executed vu: writematheader command!" in output:
            print("\n\033[1;32mMat update: SUCCESS\033[0m")
        else:
            print("\n\033[1;31mMat update: FAILED\033[0m")
            sys.exit(-1)
    else:
        print("\n\033[1;33m=== Step 3: Skipping Mat Info Update (--sn not provided) ===\033[0m")

    # 设置频率为 4800
    print("\n\033[1;36m=== Step 4: Setting Frequency to 4800 ===\033[0m")
    print("\033[1;33mSending frequency setting command...\033[0m")
    send_chunked_command(s, "cfgfreq 4800")  # 使用分块发送函数

    # 输出串口回送字符
    output = ""
    while s.in_waiting > 0:
        output += s.read(s.in_waiting).decode('latin-1')
    print(output, end='')

    # 重启Board
    print("\n=== Step 5: Restarting Board ===")
    print("Turning off outlet 5...")
    os.system('curl -u admin:1234 "http://10.33.104.109/outlet?5=OFF"')
    time.sleep(1)  # 等待1秒
    print("Turning on outlet 5")
    os.system('curl -u admin:1234 "http://10.33.104.109/outlet?5=ON"')
    print("WS restart completed")

    # 持续监听串口，直到打印 "NOR task init done!"
    print("Listening to serial port...")
    s = serial.Serial(comport, 115200, timeout=1)  # 重新打开串口
    while True:
        if s.in_waiting > 0:
            output = s.read(s.in_waiting).decode('latin-1')
            # 过滤并打印关键log
            for line in output.splitlines():
                if any(keyword in line for keyword in ["Ver ", "DDR Frequency", "Training has run successfully"]):
                    print(line)
            if "NOR task init done!" in output:
                print("NOR task init done!")
                break

    # 最终确认
    print("\n=== Final Verification ===")

    # 发送 "ver" 命令
    print("\nGetting version info...")
    send_chunked_command(s, "ver")
    output = ""
    while s.in_waiting > 0:
        output += s.read(s.in_waiting).decode('latin-1')
    print(output, end='')

    # 发送 "readmatheader" 命令
    print("\nReading Mat header info...")
    send_chunked_command(s, "readmatheader")
    output = ""
    while s.in_waiting > 0:
        output += s.read(s.in_waiting).decode('latin-1')
    print(output, end='')

    # 发送 "showcfginfo" 命令
    print("\nShowing config info...")
    send_chunked_command(s, "showcfginfo")
    output = ""
    while s.in_waiting > 0:
        output += s.read(s.in_waiting).decode('latin-1')
    print(output, end='')

    s.close()


