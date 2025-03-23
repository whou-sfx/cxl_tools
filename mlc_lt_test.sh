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
LOG_FILE="mlc_numa1_loaded_latency_test_$(date +%Y%m%d_%H%M%S).log"

# 循环运行 MLC 测试
while [ $(($(date +%s) - START_TIME)) -lt $DURATION ]; do
   sudo numactl --membind=1 ./mlc --loaded_latency | tee -a $LOG_FILE
   sleep 10  # 每次测试间隔 10 秒
done
