#########################################################################
# File Name: mlc_lt_test.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2025年03月23日 星期日 16时28分01秒
# Log: 
#########################################################################
#!/bin/bash



# 设置测试时长（秒）
DURATION=36000
START_TIME=$(date +%s)

# 生成带日期的日志文件名
LOG_FILE="mlc_numa1_test_$(date +%Y%m%d_%H%M%S).log"

# 初始化 loop count
LOOP_COUNT=1


sudo su -c "echo 4000 > /proc/sys/vm/nr_hugepages"

# 循环运行 MLC 测试
while [ $(($(date +%s) - START_TIME)) -lt $DURATION ]; do
    # 打印当前 loop count 和时间戳
    echo "===== Loop $LOOP_COUNT | Timestamp: $(date +"%Y-%m-%d %H:%M:%S") =====" | tee -a $LOG_FILE
    
    # 运行 MLC 带宽测试
    #
    #sudo numactl --membind=1 --cpunodebind=1 ./mlc --bandwidth_matrix | tee -a $LOG_FILE
    sudo numactl --membind=1 ./mlc --loaded_latency | tee -a $LOG_FILE

    # 增加 loop count
    LOOP_COUNT=$((LOOP_COUNT + 1))
    
    # 每次测试间隔 10 秒
    sleep 10
done

