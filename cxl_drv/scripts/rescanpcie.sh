#!/usr/bin/env bash


for bus_id in $(lspci -d cc53: | awk '{print $1}'); do
echo "remove pcie $bus_id"
sudo su -c "echo 1 >/sys/bus/pci/devices/0000:$bus_id/remove"
done
sleep 2
echo "rescan pcie"
sudo su -c "echo 1 >/sys/bus/pci/rescan"
echo "rescan done"
lspci -d cc53:
sleep 3
