#########################################################################
# File Name: switch_driver.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2025年11月05日 星期三 17时49分00秒
# Log: 
#########################################################################
#!/bin/bash

# Script purpose: Detect PCI devices with vendor ID cc53 and switch drivers based on the provided parameter
# Usage: ./switch_driver.sh <driver_name> [-i] where driver_name is "doe" or "cxl_pci", and -i ignores the driver load check

if [ $# -lt 1 ]; then  # Now allow for an optional second argument
    echo "Error: Please provide at least one argument (doe or cxl_pci)"
    echo "Optional: Add -i to ignore the driver load check"
    exit 1
fi

TARGET_DRIVER=$1  # First argument: doe or cxl_pci
IGNORE_CHECK=0    # Default: Do not ignore the check

# Check for the ignore flag as the second argument
if [ $# -eq 2 ] && [ "$2" = "-i" ]; then
    IGNORE_CHECK=1  # Set flag to ignore the check
    echo "Warning: Driver load check is being ignored as per flag."
fi

# Check if the target driver is loaded, but only if IGNORE_CHECK is not set
if [ "$IGNORE_CHECK" -eq 0 ] && ! lsmod | grep -q "$TARGET_DRIVER"; then
    echo "Error: The $TARGET_DRIVER driver is not loaded. Please load it with 'sudo modprobe $TARGET_DRIVER' and try again."
    exit 1
fi

# Find devices with vendor ID cc53 and prepend "0000:" to BDF
DEVICES=$(lspci -d cc53: | awk '{print "0000:" $1}')  # Updated to use lspci -d cc53:

if [ -z "$DEVICES" ]; then
    echo "Error: No PCI devices with vendor ID cc53 found"
    exit 1
fi

for BDF in $DEVICES; do  # Iterate through each device
    echo "Processing device: $BDF"
    
    # First, attempt to unbind the current driver
    echo -n "$BDF" | tee /sys/bus/pci/drivers/cxl_pci/unbind > /dev/null 2>&1  # Try unbinding cxl_pci
    echo -n "$BDF" | tee /sys/bus/pci/drivers/doe/unbind > /dev/null 2>&1     # Try unbinding doe (if bound)
    
    if [ "$TARGET_DRIVER" = "doe" ]; then
        # Bind to doe driver
        echo -n "$BDF" | tee /sys/bus/pci/drivers/doe/bind
        echo "Device $BDF has been bound to the doe driver"
    elif [ "$TARGET_DRIVER" = "cxl_pci" ]; then
        # Bind to cxl_pci driver
        echo -n "$BDF" | tee /sys/bus/pci/drivers/cxl_pci/bind
        echo "Device $BDF has been bound to the cxl_pci driver"
    else
        echo "Error: Invalid argument. Only 'doe' or 'cxl_pci' are supported"
        exit 1
    fi
done

echo "Driver switching complete. Please use 'lspci -dcc53: -v' to check the device status."
