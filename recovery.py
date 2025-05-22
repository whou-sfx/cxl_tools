##!/usr/bin/python3

#need install the lib
# sudo pip3 install PySerial, pexpect_serial,  pyprind, xmodem

import os, sys, time
import subprocess
import pkg_resources


# 检查 python3 是否安装
def check_python3():
    try:
        subprocess.check_call(['python3', '--version'])
    except:
        print("Error: python3 is not installed!")
        print("Please install python3 first:")
        print("sudo apt update && sudo apt install python3")
        sys.exit(1)

# 检查 pip3 是否安装
def check_pip3():
    try:
        subprocess.check_call(['python3', '-m', 'pip', '--version'])
    except:
        print("Error: pip3 is not installed!")
        print("Please install pip3 first:")
        print("sudo apt update && sudo apt install python3-pip")
        sys.exit(1)

# 先检查 python3 和 pip3
check_python3()
check_pip3()

# 检查并安装所需库
def check_and_install_packages():
    # 包名与导入名的映射
    required_packages = {
        'serial': 'pyserial',
        'pexpect_serial': 'pexpect_serial',
        'pyprind': 'pyprind',
        'xmodem': 'xmodem'
    }
    
    for import_name, package_name in required_packages.items():
        try:
            __import__(import_name)
        except ImportError:
            print(f"{import_name} is not installed, installing {package_name}...")
            # 使用 sudo -H 确保正确的权限
            subprocess.check_call(['sudo', '-H', sys.executable, '-m', 'pip', 'install', package_name])
        
        # 再次尝试导入，确保安装成功
        try:
            __import__(import_name)
        except ImportError as e:
            print(f"Failed to import {import_name} after installing {package_name}: {e}")
            print("Please try to install it manually:")
            print(f"sudo -H {sys.executable} -m pip install {package_name}")
            sys.exit(1)

# 检查并安装其他依赖
check_and_install_packages()


import serial
from pexpect_serial import SerialSpawn
from optparse import OptionParser
import xmodem
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
    # 逐个字符发送
    for char in cmd:
        s.write(char.encode())
        time.sleep(0.1)  # 每个字符发送后休眠 0.1 秒
    s.write("\r\n".encode())  # 发送换行符
    time.sleep(1)  # 等待 1 秒以确保设备处理命令
    try:
        ret = s.read(32)
        strOut = (ret.decode())
    except:
        pass
    return strOut

def upgrade_process(cmd, bin_path):
    global bar  # 定义 bar 为全局变量
    statinfo_bin = os.stat(bin_path)
    bar = pyprind.ProgBar(statinfo_bin.st_size/128+2)  # 初始化进度条
    comOutput = sendbin(cmd)
    while "NCCC" not in comOutput:
        comOutput = sendbin(cmd)
    
    stream = open(bin_path, 'rb')
    m = xmodem.XMODEM(getc, putc)
    m.send(stream)
    print(f"\n{cmd} dnld done")

    # 串口打印回显，直到最后输出不为 "DDR data init start" 并且1秒内无输出
    last_output_time = time.time()
    last_output = ""
    while True:
        if s.in_waiting > 0:
            output = s.read(s.in_waiting).decode('utf-8', errors='ignore')
            print(output, end='', flush=True)
            last_output = output  # 更新最后输出
            last_output_time = time.time()
        else:
            # 如果最后输出不为 "DDR data init start" 并且1秒内无输出，则退出
            if "DDR data init start" not in last_output and time.time() - last_output_time > 1:
                break
        time.sleep(0.1)  # 避免CPU占用过高
    print("\nimage push success!")

if __name__ == '__main__':

    logging.basicConfig()
    parser = OptionParser(usage="python %prog [options]")
    parser.add_option("-f", dest="bin_path", help="path of Firmware")
    parser.add_option("-l", dest="cmd_type", type="int", default=1, help="1: recovery from rom, 2: update new from firmware")

    (opt, args) = parser.parse_args()

    if not os.path.exists(opt.bin_path):
        print("\n %s, Error: File [ %s ] not found !!!\n" % (sys.stderr, opt.bin_path))
        parser.print_help()
        sys.exit(-1)

    print("kill minicom")
    os.system("sudo killall -9 minicom")
    s = serial.Serial(comport,115200,timeout=1)

    if opt.cmd_type == 1:
        upgrade_process("rdl", opt.bin_path)
        upgrade_process("ndl2", opt.bin_path)
    else:
        upgrade_process("ndl2", opt.bin_path)

    s.close()

