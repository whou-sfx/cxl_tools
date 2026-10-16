#########################################################################
# File Name: /work/document/scripts/cxl_drv_enable_dbg.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2026年06月16日 星期二 16时44分04秒
# Log: 
#########################################################################
#!/bin/bash
echo 'module cxl_core +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
echo 'module cxl_pci +p'  | sudo tee /sys/kernel/debug/dynamic_debug/control
echo 'module cxl_port +p'  | sudo tee /sys/kernel/debug/dynamic_debug/control
echo 'module cxl_mem +p'  | sudo tee /sys/kernel/debug/dynamic_debug/control
