#!/bin/bash
#########################################################################
# File Name: power_cycle_test.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2025年03月21日 星期五 21时25分51秒
# Log: 
#########################################################################
LOG_FILE="/var/log/power_cycle.log"
CYCLE_COUNT_FILE="/var/log/power_cycle_count.txt"

# 初始化循环计数
if [ -f "$CYCLE_COUNT_FILE" ]; then
    CYCLE_COUNT=$(cat "$CYCLE_COUNT_FILE")
else
    CYCLE_COUNT=0
    echo "$CYCLE_COUNT" > "$CYCLE_COUNT_FILE"
fi

# 记录日志函数
log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - LOOP_$CYCLE_COUNT - $1" >> $LOG_FILE
    echo "$(sudo lspci -d cc53: -vvv | grep LnkSta:)" >>$LOG_FILE
    sync
    sleep 5
}

# 检查设备是否存在
check_device() {
    # 使用sudo lspci命令获取CXL设备详细信息
    local cxl_info=$(sudo lspci -d cc53: -vvvv 2>/dev/null)
    
    # 检查CXLCtl中是否包含"Mem+"
    if echo "$cxl_info" | grep -q "CXLCtl:.*Mem+"; then
        return 0  # Mem+已配置
    else
        return 1  # Mem+未配置
    fi
}

# 主循环
while true; do
    if check_device; then
        CYCLE_COUNT=$((CYCLE_COUNT + 1))
        echo "$CYCLE_COUNT" > "$CYCLE_COUNT_FILE"  # 将计数写回文件
        log_message "CXL DVSEC ID 0 memor Enabled, probe success, power cycle"
        sleep 120
        sudo ipmitool power cycle
        sleep 60 # 等待系统重启
    else
        rm $CYCLE_COUNT_FILE
        log_message "DVSEC ID 0 mem_en not found"
        break
    fi
done
