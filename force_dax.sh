sudo su -c "echo 0 >/sys/bus/dax/drivers_autoprobe"
sudo modprobe cxl_pci
sudo ./numa_to_dax.sh
