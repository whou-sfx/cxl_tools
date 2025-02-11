#########################################################################
# File Name: dax_to_numa.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2024年08月21日 星期三 19时58分33秒
# Log: 
#########################################################################
#!/bin/bash
# show cxl state
sudo daxctl list

# show numa info
sudo numactl -H

# convert dax mode to numa mode
sudo daxctl reconfigure-device --mode=system-ram dax0.0 --force

#show numa info and cxl state again
sudo numactl -H

sudo daxctl list



