#!/bin/bash

# 检查是否提供了输入参数
if [ -z "$1" ]; then
    echo "Usage: $0 <output_name>"
    exit 1
fi

# 获取当前时间
current_time=$(date +"%Y%m%d_%H%M%S")

# 定义输出文件名
output_file="${1}_${current_time}.txt"

# 定义delay的取值范围，避免重复
delays=($(seq 600 25 850) $(seq 875 50 1000) $(seq 1100 100 3500))

# 清空或创建输出文件
> "$output_file"

# 遍历每个delay值
for delay in "${delays[@]}"; do
    # 运行mlc命令并获取最后一行
    output=$(sudo ./mlc --loaded_latency -j1 -k1-15 -d$delay | tail -n 1)
    
    # 如果获取到有效数据，则追加到输出文件
    if [[ -n $output ]]; then
        echo "$output" >> "$output_file"
        echo "Delay: $delay, Data: $output"
    fi
done

echo "Results saved to $output_file" 