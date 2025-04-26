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

# 绘制点集和趋势曲线
def plot_data(bandwidth, latency):
    plt.figure(figsize=(10, 6))
    
    # 绘制点集
    plt.scatter(bandwidth, latency, color='blue', label='Data Points')
    
    # 绘制趋势曲线（使用多项式拟合）
    z = np.polyfit(bandwidth, latency, 3)  # 3次多项式拟合
    p = np.poly1d(z)
    bandwidth_sorted = np.sort(bandwidth)
    plt.plot(bandwidth_sorted, p(bandwidth_sorted), color='red', label='Trend Curve')
    
    # 添加标签和标题
    plt.xlabel('Bandwidth (MB/s)')
    plt.ylabel('Latency (ns)')
    plt.title('Latency vs Bandwidth')
    plt.legend()
    plt.grid(True)
    plt.show()

# 主函数
def main():
    # 检查是否提供了输入参数
    if len(sys.argv) < 2:
        print("Usage: python3 plot_latency_bandwidth.py <file_path>")
        sys.exit(1)
    
    file_path = sys.argv[1]  # 从命令行参数获取文件路径
    bandwidth, latency = read_data(file_path)
    plot_data(bandwidth, latency)

if __name__ == "__main__":
    main() 