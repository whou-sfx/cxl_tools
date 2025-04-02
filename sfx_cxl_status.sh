#!/bin/bash

# 高亮处理函数
highlight_status() {
    local status_info=$1
    local highlighted=""
    # 高亮非+的值
    for item in $status_info; do
        if [[ $item == *"-" ]]; then
            highlighted+="\033[31m$item\033[0m "  # 高亮为红色
        else
            highlighted+="$item "
        fi
    done
    echo -n "$highlighted"
}

# Filter PCIe devices that meet the criteria
devices=$(lspci -d cc53::0502 | awk '{print $1}')

for bdf in $devices; do
    
    # Get detailed lspci information
    lspci_output=$(lspci -s $bdf -vvvv)
    
    # 2.1 Print CXL Link negotiation information
    fbsta=$(echo "$lspci_output" | grep -A 3 "Vendor=1e98 ID=0007" | grep "FBSta:")
    fbmodts=$(echo "$lspci_output" | grep -A 4 "Vendor=1e98 ID=0007" | grep "FBModTS:" | awk '{print $NF}')
    
    # Extract relevant information
    link_info=$(echo "$fbsta" | awk '{for(i=1;i<=NF;i++) if($i ~ /IO|Mem|68BFlit/) printf "%s ", $i; }')
    link_info=$(highlight_status "$link_info")
    
    # 2.2 Print CXL Memory information
    hdmcount=$(echo "$lspci_output" | grep -A 3 "Vendor=1e98 ID=0000" | grep -oP 'HDMCount \K[0-9]+')
    cxlctl=$(echo "$lspci_output" | grep -A 3 "Vendor=1e98 ID=0000" | grep "CXLCtl:")
 
    # Process CXLCtl to extract relevant information
    cxlctl_info=$(echo "$cxlctl" | awk '{for(i=1;i<=NF;i++) if($i ~ /IO|Mem|Viral/) printf "%s ", $i; }')
    cxlctl_info=$(highlight_status "$cxlctl_info")
    
    echo -e "SFX CXL Device: $bdf"
    echo -e "CXL Link Nego: $link_info ModTS: $fbmodts"

    echo -e "CXL Mem info:"
    echo -e "  CXLCtl: $cxlctl_info"  # Use -e to enable interpretation of backslash escapes
    echo -e "  HDMCount: $hdmcount"
    # Iterate over each Range and calculate size in GB
    ranges=$(echo "$lspci_output" | grep -A 10 "Vendor=1e98 ID=0000" | grep "Range")
    count=0  # Initialize a counter
    while IFS= read -r range && [ $count -lt $hdmcount ]; do
        range=$(echo "$range" | sed 's/^[ |\t]*//')  # Remove leading whitespace and tabs
        range_start=$(echo "$range" | awk -F'[- ]' '{print $2}' | sed 's/^0*//')  # Remove leading zeros
        range_end=$(echo "$range" | awk -F'[- ]' '{print $3}' | sed 's/^0*//')      # Remove leading zeros

        # Check if the values are empty after removing leading zeros
        range_start=${range_start:-0}  # Default to 0 if empty
        range_end=${range_end:-0}      # Default to 0 if empty

        # Convert hex to decimal
        start_dec=$((0x$range_start))
        end_dec=$((0x$range_end))
        
        # Calculate size in GB
        size_gb=$(( (end_dec - start_dec + 1) / (1024 * 1024 * 1024) ))  # +1 to include the start address
        
        range_info=$(echo "$range" | awk -F'[: -]' '{print $2, $3, $4}')

        status=$(echo "$lspci_output" | grep -A 1 "$range" | grep "Valid")
        status_info=$(echo "$status" | awk '{for(i=1;i<=NF;i++) if($i ~ /Valid|Active/) printf "%s ", $i; }')
        status_info=$(highlight_status "$status_info")

        # Ensure the output is correctly formatted
        echo -e "  $range, Size: ${size_gb}GB, $status_info"

        count=$((count + 1))  # 增加计数器
    done <<< "$ranges"
    
    echo ""
done 
