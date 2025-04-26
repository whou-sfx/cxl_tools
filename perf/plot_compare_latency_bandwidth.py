import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

# 读取文件
def read_data(file_path):
    # 读取文件，假设文件是以空格分隔的
    df = pd.read_csv(file_path, delim_whitespace=True, header=None)
    # 提取第三列（带宽）作为横坐标，第二列（时延）作为纵坐标
    bandwidth = df[2]  # 第三列
    latency = df[1]    # 第二列
    return bandwidth, latency

# 绘制趋势曲线
def plot_trend(bandwidth, latency, label, color):
    # 使用多项式拟合
    z = np.polyfit(bandwidth, latency, 3)  # 3次多项式拟合
    p = np.poly1d(z)
    bandwidth_sorted = np.sort(bandwidth)
    plt.plot(bandwidth_sorted, p(bandwidth_sorted), color=color, label=label)

# 主函数
def main():
    # 检查是否提供了两个输入参数
    if len(sys.argv) < 3:
        print("Usage: python3 plot_compare_latency_bandwidth.py <file1> <file2>")
        sys.exit(1)
    
    # 读取两个文件的数据
    file1 = sys.argv[1]
    file2 = sys.argv[2]
    bandwidth1, latency1 = read_data(file1)
    bandwidth2, latency2 = read_data(file2)
    
    # 创建图形
    plt.figure(figsize=(10, 6))
    
    # 绘制第一组数据的趋势曲线（红色）
    plot_trend(bandwidth1, latency1, label='MCM500', color='red')
    
    # 绘制第二组数据的趋势曲线（蓝色）
    plot_trend(bandwidth2, latency2, label='3rdparty', color='blue')
    
    # 添加标签和标题
    plt.xlabel('Bandwidth (MB/s)')
    plt.ylabel('Latency (ns)')
    plt.title('Latency vs Bandwidth Comparison')
    plt.legend()
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    main() 