#########################################################################
# File Name: fw_update.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2025年05月22日 星期四 21时16分42秒
# Log: 
#########################################################################
#!/bin/bash

img=$1
echo 1 > /sys/class/firmware/mem0/loading
cat $img > /sys/class/firmware/mem0/data
echo 0 > /sys/class/firmware/mem0/loading
printf "wait for transfer-fw done...(sleep 10s)\n"
sleep 10
remaining_size=$(cat /sys/class/firmware/mem0/remaining_size)
if [[ $remaining_size == 0 ]]; then
	printf "####################### transfer-fw success #######################\n"
	rc=0
else
	printf "####################### transfer-fw failed!!! #######################\n"
	rc=-1
fi

