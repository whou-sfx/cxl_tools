#########################################################################
# File Name: check_sfx_cxl_dev.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2025年03月21日 星期五 20时00分36秒
# Log: 
#########################################################################
#!/bin/bash
#!/bin/bash

# 函数：检查CXLCtl中的Mem+是否配置
check_cxl_mem_config() {
    # 使用sudo lspci命令获取CXL设备详细信息
    local cxl_info=$(sudo lspci -d cc53: -vvvv 2>/dev/null)
    
    # 检查CXLCtl中是否包含"Mem+"
    if echo "$cxl_info" | grep -q "CXLCtl:.*Mem+"; then
        return 0  # Mem+已配置
    else
        return 1  # Mem+未配置
    fi
}

# 使用示例
if check_cxl_mem_config; then
    echo "CXLCtl中的Mem+已配置"
else
    echo "CXLCtl中的Mem+未配置"
fi
