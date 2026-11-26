#!/bin/bash
#########################################################################
# File Name: fw_hot_active.sh
# Desc: transfer-fw (2K chunk) + activate-fw via mbcci-sfx (MB-CCI ioctl)
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2026年10月16日
# Log:
#########################################################################

MBCCI=/home/tcn/whou/libcxlmi/./build/tools/mbcci-sfx/mbcci-sfx
CHUNK_SIZE=2048
# Kernel rejects RAW ACTIVATE_FW (0x0202) unless raw_allow_all is set
RAW_ALLOW_ALL=/sys/kernel/debug/cxl/mbox/raw_allow_all

usage()
{
	echo "Usage: $0 --input <fw_file> --slot <n> --action <online|offline> [--dev <memN>]"
	echo "  --dev    CXL memdev under /dev/cxl/ (default: mem0)"
	exit 1
}

dev=mem0
input=
slot=
action=

while [[ $# -gt 0 ]]; do
	case $1 in
	--input)  input=$2;  shift 2 ;;
	--slot)   slot=$2;   shift 2 ;;
	--action) action=$2; shift 2 ;;
	--dev)    dev=$2;    shift 2 ;;
	-h|--help) usage ;;
	*) echo "Unknown option: $1"; usage ;;
	esac
done

[[ -z $input || -z $slot || -z $action ]] && usage
[[ $action != online && $action != offline ]] && { echo "--action must be online or offline"; usage; }
[[ -f $input ]] || { echo "fw file not found: $input"; exit 1; }
[[ -x $MBCCI ]] || { echo "mbcci-sfx not found: $MBCCI"; exit 1; }

printf "####################### get-fw-info (before) #######################\n"
$MBCCI $dev get-fw-info

printf "####################### transfer-fw: %s -> slot %s #######################\n" "$input" "$slot"
if ! $MBCCI $dev transfer-fw --input "$input" --slot "$slot" --chunk-size $CHUNK_SIZE; then
	printf "####################### transfer-fw failed!!! #######################\n"
	exit 1
fi

# Temporarily allow RAW ACTIVATE_FW, restore the original value afterwards
mountpoint -q /sys/kernel/debug || mount -t debugfs none /sys/kernel/debug
if [[ ! -w $RAW_ALLOW_ALL ]]; then
	echo "cannot write $RAW_ALLOW_ALL (need root / debugfs / no lockdown)"
	exit 1
fi
raw_orig=$(cat $RAW_ALLOW_ALL)
echo Y > $RAW_ALLOW_ALL

sleep 12

printf "####################### activate-fw: slot %s %s #######################\n" "$slot" "$action"
$MBCCI $dev activate-fw --slot "$slot" --action "$action"
rc=$?

echo "$raw_orig" > $RAW_ALLOW_ALL

if [[ $rc -ne 0 ]]; then
	printf "####################### activate-fw failed!!! #######################\n"
	exit 1
fi

printf "####################### get-fw-info (after) #######################\n"
$MBCCI $dev get-fw-info
printf "####################### fw %s activate success #######################\n" "$action"
exit 0
