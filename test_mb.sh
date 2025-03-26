#!/bin/bash

loop=$1

# 获取设备的 PCI 地址（BDF）
DEVICE=$(lspci -d cc53: | awk '{print $1}')

# 检查是否找到设备
if [ -z "$DEVICE" ]; then
    echo "未找到设备：cc53"
    exit 1
fi

# 打印设备的 BDF 信息
echo "设备的 BDF 信息为：$DEVICE"

# 获取 Region 2 的基地址
BAR2=$(lspci -s $DEVICE -vvv | grep -i "Region 2" | awk '{print $5}')

# 检查是否找到 BAR2
if [ -z "$BAR2" ]; then
    echo "未找到 BAR2 基地址"
    exit 1
fi

# 输出 BAR2 基地址
echo "设备 $DEVICE 的 BAR2 基地址为：0x$BAR2"

bar2_base=0x$BAR2

((mem_dev_reg_base = bar2_base + 0x10000))
payload_sz=2048
mb_start=0x100
payload_start=0x120
((payload_end = payload_start + payload_sz))

err_cnt=0

function test_mb_multi_wr() {
	local start=$1
	local end=$2
	local n_bits=$3
	printf "============ [start] %d bits multi write test: 0x%x-0x%x ==========\n" $n_bits $start $end
	for ((offset = start; offset < end; offset += n_bits / 8)); do
		(( addr = mem_dev_reg_base + offset))
		if [[ $n_bits == 8 || $n_bits == 16 ]]; then
			(( value = (~(offset + loop)) & ((1 << n_bits) - 1) ))
		elif [[ $n_bits == 32 ]]; then
			(( value = (~(((offset << 16) | offset) + loop) ) & 0xffffffff ))
		elif [[ $n_bits == 64 ]]; then
			(( value = (~(((offset << 48) | (offset << 32) | (offset << 16) | offset) + loop) ) & 0x7fffffffffffffff ))
		fi
		busybox devmem $addr $n_bits $value
		printf "[loop %8s] [0x%x]: %#16llx (write value)\n" $loop $addr $value
	done
	for ((offset = start; offset < end; offset += n_bits / 8)); do
		(( addr = mem_dev_reg_base + offset))
		if [[ $n_bits == 8 || $n_bits == 16 ]]; then
			(( value = (~(offset + loop)) & ((1 << n_bits) - 1) ))
		elif [[ $n_bits == 32 ]]; then
			(( value = (~(((offset << 16) | offset) + loop) ) & 0xffffffff ))
		elif [[ $n_bits == 64 ]]; then
			(( value = (~(((offset << 48) | (offset << 32) | (offset << 16) | offset) + loop) ) & 0x7fffffffffffffff ))
		fi
		(( value_read_back = $(busybox devmem $addr $n_bits) ))
		if [[ $value_read_back == $value ]]; then
			printf "[loop %8s] [0x%x]: %#16llx ( read back ) [success]\n" $loop $addr $value_read_back
		else
			(( err_cnt++ ))
			printf "[loop %8s] [0x%x]: %#16llx ( read back ) [err, data mismatch! expected value: %#16llx]\n" $loop $addr $value_read_back $value
		fi
	done
	printf "============ [ end ] %d bits multi write test: 0x%x-0x%x ==========\n" $n_bits $start $end
}

function test_mb_wr_rd() {
	local start=$1
	local end=$2
	local n_bits=$3
	printf "============ [start] %d bits write+read test: 0x%x-0x%x ==========\n" $n_bits $start $end
	for ((offset = start; offset < end; offset += n_bits / 8)); do
		(( addr = mem_dev_reg_base + offset))
		if [[ $n_bits == 8 || $n_bits == 16 ]]; then
			(( value = (offset + loop) & ((1 << n_bits) - 1) ))
		elif [[ $n_bits == 32 ]]; then
			(( value = (((offset << 16) | offset) + loop) & 0xffffffff ))
		elif [[ $n_bits == 64 ]]; then
			(( value = (((offset << 48) | (offset << 32) | (offset << 16) | offset) + loop) & 0x7fffffffffffffff ))
		fi
		busybox devmem $addr $n_bits $value
		(( value_read_back = $(busybox devmem $addr $n_bits) ))
		printf "[loop %8s] [0x%x]: %#16llx (write value)\n" $loop $addr $value
		if [[ $value_read_back == $value ]]; then
			printf "[loop %8s] [0x%x]: %#16llx ( read back ) [success 1st write]\n" $loop $addr $value_read_back
		else
			printf "[loop %8s] [0x%x]: %#16llx ( read back ) [try write again]\n" $loop $addr $value_read_back
			(( err_cnt++ ))
			busybox devmem $addr $n_bits $value
			(( value_read_back = $(busybox devmem $addr $n_bits) ))
			if [[ $value_read_back == $value ]]; then
				printf "[loop %8s] [0x%x]: %#16llx ( read back ) [success after 2nd write]\n" $loop $addr $value_read_back
			else
				printf "[loop %8s] [0x%x]: %#16llx ( read back ) [try write again]\n" $loop $addr $value_read_back
				busybox devmem $addr $n_bits $value
				(( value_read_back = $(busybox devmem $addr $n_bits) ))
				if [[ $value_read_back == $value ]]; then
					printf "[loop %8s] [0x%x]: %#16llx ( read back ) [success after 3rd write]\n" $loop $addr $value_read_back
				else
					printf "[loop %8s] [0x%x]: %#16llx ( read back ) [error after 3rd write !!!!!!]\n" $loop $addr $value_read_back
				fi
			fi
		fi
	done
	printf "============ [ end ] %d bits write+read test: 0x%x-0x%x ==========\n" $n_bits $start $end
}


printf "BAR2 mb\n"
#printf "============ mailbox cap,ctrl,cmd,sts,bgcmdsts ==========\n"
#test_mb_wr_rd $mb_start $payload_start 32

printf "####################### mailbox payload: write+read #######################\n"
test_mb_wr_rd $payload_start $payload_end 8
test_mb_wr_rd $payload_start $payload_end 16
test_mb_wr_rd $payload_start $payload_end 32
test_mb_wr_rd $payload_start $payload_end 64

printf "####################### mailbox payload: multi write #######################\n"
test_mb_multi_wr $payload_start $payload_end 8
test_mb_multi_wr $payload_start $payload_end 16
test_mb_multi_wr $payload_start $payload_end 32
test_mb_multi_wr $payload_start $payload_end 64

if [[ $err_cnt == 0 ]]; then
	printf "####################### test mailbox payload pass #######################\n\n"
	rc=0
else
	printf "####################### test mailbox payload fail, err_cnt:%d!!! #######################\n\n" $err_cnt
	rc=-1
fi

printf "####################### transfer-fw, loop: %d #######################\n" $loop
echo 1 > /sys/class/firmware/mem0/loading
cat /home/tcn/jurian/bl2.bin > /sys/class/firmware/mem0/data
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

exit $rc
