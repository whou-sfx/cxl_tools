#########################################################################
# File Name: acpi_srat_check.sh
# Desc:
# Author: Andy-wei.hou
# mail: wei.hou@scaleflux.com
# Created Time: 2024年09月23日 星期一 20时04分27秒
# Log: 
#########################################################################
#!/bin/bash
# Install packages
#$ sudo apt install acpica-tools
# Extract ACPI Tables
sudo acpidump -o acpidump.out
# Separate Dumped files by tables
acpixtract -a acpidump.out
# Change raw data's format to human-readable through parser
iasl -d srat.dat
# Find the result
ls srat.dsl
#srat.dsl


# Then Using the "Base Address" that FW get from Device to get the Entry
#
# Or by checkign the dmesg by
#  ACPI: SRAT: Node ...... [mem xxxxxx - xxxxxxxx]
