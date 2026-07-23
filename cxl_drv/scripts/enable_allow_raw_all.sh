#########################################################################
# File Name: enable_allow_raw_all.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2026年07月23日 星期四 16时51分22秒
# Log: 
#########################################################################
#!/bin/bash
sudo su -c "echo 1 >/sys/kernel/debug/cxl/mbox/raw_allow_all"
