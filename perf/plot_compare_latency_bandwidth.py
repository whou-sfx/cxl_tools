import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
from scipy.interpolate import make_interp_spline

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
    # 对带宽进行排序
    sorted_idx = np.argsort(bandwidth)
    bandwidth_sorted = bandwidth[sorted_idx]
    latency_sorted = latency[sorted_idx]
    
    # 生成平滑曲线
    spline = make_interp_spline(bandwidth_sorted, latency_sorted, k=3)  # 使用3次样条
    bandwidth_smooth = np.linspace(bandwidth_sorted.min(), bandwidth_sorted.max(), 500)
    latency_smooth = spline(bandwidth_smooth)
    
    plt.plot(bandwidth_smooth, latency_smooth, color=color, label=label)

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