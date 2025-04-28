#!/bin/bash

# ------------------------------------------------------------------------------------------------
# Utility Functions
# ------------------------------------------------------------------------------------------------

function require_tool() {
    command -v "$1" >/dev/null 2>&1 || { echo "Error: '$1' not found in PATH"; exit 1; }
}

# ------------------------------------------------------------------------------------------------
# Pre-checks
# ------------------------------------------------------------------------------------------------

require_tool lspci
require_tool uname

# ------------------------------------------------------------------------------------------------
# System Info
# ------------------------------------------------------------------------------------------------
function print_system_info() {
    decoder=$(ls -d /sys/bus/cxl/devices/decoder*/ 2>/dev/null | head -n 1)
    if [[ -z "$decoder" ]]; then
        echo "No decoder device found."
        return
    fi

    decname=$(basename "$decoder")
    start="N/A"
    size_hex="N/A"
    size_gb="N/A"

    [[ -f "${decoder}start" ]] && start=$(cat "${decoder}start")
    if [[ -f "${decoder}size" ]]; then
        size_hex=$(cat "${decoder}size")
        size_gb=$((size_hex / 1024 / 1024 / 1024))
    fi

    echo "=== System Info ==="
    echo "Vendor ID        : $(cat /sys/class/dmi/id/board_vendor 2>/dev/null || echo "N/A")"
    echo "Kernel Version   : $(uname -r)"
    echo "BIOS Version     : $(cat /sys/class/dmi/id/bios_version 2>/dev/null || echo "N/A")"
    echo "Decoder         : $decname"
    echo "  Start Address : $start"
    echo "  Size          : ${size_gb} GB"
    echo ""
}

# ------------------------------------------------------------------------------------------------
# Analyze CXL devices via lspci
# ------------------------------------------------------------------------------------------------
function analyze_sfx_cxl_devices() {
    local devices
    devices=$(lspci -d cc53::0502 | awk '{print $1}')

    echo "=== Device Info ==="
    for bdf in $devices; do
        lspci_output=$(lspci -s "$bdf" -vvvv)

        link_status=$(echo "$lspci_output" | grep "LnkSta:" | sed -E 's/.*LnkSta:[[:space:]]+//')
        fbsta=$(echo "$lspci_output" | grep "FBSta:" | sed -E 's/.*FBSta:[[:space:]]+//')
        fbmodts=$(echo "$lspci_output" | grep -A 4 "Vendor=1e98 ID=0007" | grep "FBModTS:" | awk '{print $NF}')

        hdmcount=$(echo "$lspci_output" | grep -A 3 "Vendor=1e98 ID=0000" | grep -oP 'HDMCount \K[0-9]+')
        cxlctl=$(echo "$lspci_output" | grep -A 3 "Vendor=1e98 ID=0000" | grep "CXLCtl:")
        cxlctl_info=$(echo "$cxlctl" | awk '{for(i=1;i<=NF;i++) if($i ~ /IO|Mem|Viral/) printf "%s ", $i; }')

        numa_node=$(cat /sys/bus/pci/devices/0000:${bdf}/numa_node 2>/dev/null || echo "N/A")

        mem_device=$(readlink -f /sys/bus/cxl/devices/mem*/ | grep "$bdf" | xargs -n1 basename 2>/dev/null | head -n1)
        mem_device=${mem_device:-N/A}

        mem_path="/sys/bus/cxl/devices/${mem_device}/"
        ram_size_raw=$(cat "${mem_path}ram/size" 2>/dev/null || echo "")
        pmem_size_raw=$(cat "${mem_path}pmem/size" 2>/dev/null || echo "")

        dax_path=$(find /sys/bus/dax/devices/dax* -maxdepth 0 -type l 2>/dev/null | head -n1)
        dax_node="N/A"
        numa_node="N/A"

        if [[ -n "$dax_path" && -e "$dax_path" ]]; then
            dax_node=$(basename "$dax_path")
            target_node_path="${dax_path}/target_node"
            [[ -f "$target_node_path" ]] && numa_node=$(cat "$target_node_path")
        fi

        # Parse RAM size
        if [[ "$ram_size_raw" =~ ^0x[0-9a-fA-F]+$ ]]; then
            ram_size_bytes=$((ram_size_raw))
            ram_size_gb=$((ram_size_bytes / 1024 / 1024 / 1024))
        elif [[ "$ram_size_raw" =~ ^[0-9]+$ ]]; then
            ram_size_bytes=$ram_size_raw
            ram_size_gb=$((ram_size_bytes / 1024 / 1024 / 1024))
        else
            ram_size="N/A"
        fi

        # Parse PMEM size
        if [[ "$pmem_size_raw" =~ ^0x[0-9a-fA-F]+$ ]]; then
            pmem_size_bytes=$((pmem_size_raw))
            pmem_size_gb=$((pmem_size_bytes / 1024 / 1024 / 1024))
        elif [[ "$pmem_size_raw" =~ ^[0-9]+$ ]]; then
            pmem_size_bytes=$pmem_size_raw
            pmem_size_gb=$((pmem_size_bytes / 1024 / 1024 / 1024))
        else
            pmem_size="N/A"
        fi
        fw_version=$(cat ${mem_path}firmware_version 2>/dev/null || echo "N/A")

        echo "SFX CXL Device   : ${mem_device}"
        echo "FW Version       : $fw_version"
        echo "PCIe Address     : $bdf"
        echo -e "PCIe Link Status : $link_status"
        echo -e "FBSta            : $fbsta"
        echo "RAM Size         : ${ram_size_gb} GB"
        echo "PMEM Size        : ${pmem_size_gb} GB"
        echo "NUMA Node        : $numa_node"
        echo "Dax Node         : $dax_node"
        echo "CXL Mem info:"
        echo -e "\tCXLCtl        : $cxlctl_info"
        echo -e "\tHDMCount      : $hdmcount"

        ranges=$(echo "$lspci_output" | grep -A 10 "Vendor=1e98 ID=0000" | grep "Range")
        count=0
        while IFS= read -r range && [ $count -lt ${hdmcount:-0} ]; do
            range=$(echo "$range" | sed 's/^[ |\t]*//')
            range_start=$(echo "$range" | awk -F'[- ]' '{print $2}' | sed 's/^0*//')
            range_end=$(echo "$range" | awk -F'[- ]' '{print $3}' | sed 's/^0*//')
            range_start=${range_start:-0}
            range_end=${range_end:-0}
            start_dec=$((0x$range_start))
            end_dec=$((0x$range_end))
            size_gb=$(( (end_dec - start_dec + 1) / (1024 * 1024 * 1024) ))

            status=$(echo "$lspci_output" | grep -A 1 "$range" | grep "Valid")
            status_info=$(echo "$status" | awk '{for(i=1;i<=NF;i++) if($i ~ /Valid|Active/) printf "%s ", $i; }')

            echo -e "\tRange         : $range, Size: ${size_gb} GB, $status_info"
            count=$((count + 1))
        done <<< "$ranges"

        echo ""
    done
}


# ------------------------------------------------------------------------------------------------
# Main Execution
# ------------------------------------------------------------------------------------------------
print_system_info
analyze_sfx_cxl_devices

exit 0
