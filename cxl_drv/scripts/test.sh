sudo modprobe -r cxl_pci cxl_port cxl_acpi cxl_mem cxl_core

sudo insmod ./cxl_core.ko
sudo insmod ./cxl_port.ko
sudo insmod ./cxl_acpi.ko
sudo insmod ./cxl_mem.ko
sudo insmod ./cxl_pci.ko
