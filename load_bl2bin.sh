echo "jtagconf 25,5
device RV32
speed 4000
connect
r0
r1
connect
loadbin bl2.bin 0x70000000
setpc 0x70000000
g
exit" > tmp_bl2.script

echo "Download BL2"
sudo JLinkExe  ./tmp_bl2.script
echo "Download BL2 Done"

rm tmp_bl2.script

#ssh tcn@10.35.48.27 "sudo reboot"
