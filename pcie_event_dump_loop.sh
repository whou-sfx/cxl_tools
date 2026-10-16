#!/usr/bin/bash
# 每 1 秒执行一次 pine_pcie_event_dump.py --dumpall，
# 每轮先打印精确到秒的时间戳，再打印脚本输出，
# 所有内容同时输出到终端并追加到当前目录下带日期的日志文件。
# 用法: ./pcie_event_dump_loop.sh [bdf] [interval_sec]
#   bdf          默认 81:00.0
#   interval_sec 默认 1

BDF="${1:-81:00.0}"
INTERVAL="${2:-1}"

cd "$(dirname "$(readlink -f "$0")")" || exit 1

LOG_FILE="./pcie_event_dump_$(date '+%Y%m%d_%H%M%S').log"
echo "log file: ${LOG_FILE}"

trap 'echo "stopped at $(date "+%Y-%m-%d %H:%M:%S")" | tee -a "${LOG_FILE}"; exit 0' INT TERM

# 启动循环前先使能所有 event
{
    echo "===== $(date '+%Y-%m-%d %H:%M:%S') enable all events ====="
    sudo python3 ./pine_pcie_event_dump.py --bdf "${BDF}" --enable all 2>&1
    echo
} | tee -a "${LOG_FILE}"
while true; do
    {
        echo "===== $(date '+%Y-%m-%d %H:%M:%S') ====="
        sudo python3 ./pine_pcie_event_dump.py --bdf "${BDF}" --dumpall 2>&1
        echo
    } | tee -a "${LOG_FILE}"
    sleep "${INTERVAL}"
done
