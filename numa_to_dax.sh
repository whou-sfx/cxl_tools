#########################################################################
# File Name: numa_to_dax.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2025年06月09日 星期一 17时16分25秒
# Log: 
#########################################################################
#!/bin/bash
sudo daxctl reconfigure-device --mode=devdax dax0.0 --force
