## Readme

脚本会通过lspci -d cc53:检查设备是否存在, 然后通过 DVSEC ID 0 中的mem_En是否置位来判断是否完成CXL probe

如果Probe成功，则通过ipmitool power cycle重启系统

每次操作都会记录到/var/log/power_cycle.log日志文件中

如果Probe失败或者设备不存在，则停止测试

> 需要lspci --version >=3.10 才能正常识别DVSEC ID 0中的mem_en



## 设置开机自启动：

~~~
# /etc/systemd/system/power_cycle_check.service
[Unit]
Description=Power Cycle Check Service
After=network.target

[Service]
ExecStart=/usr/local/bin/power_cycle_check.sh
Restart=always

[Install]
WantedBy=multi-user.target
~~~



设置服务子启动

~~~
sudo cp sudo cp power_cycle_check.service   /etc/systemd/system/
sudo cp power_cycle_test.sh /usr/local/bin/

sudo systemctl daemon-reload
sudo systemctl enable power_cycle_check.service
sudo systemctl start power_cycle_check.service
~~~


## 停止测试
可以通过 Jlink r0 将测试停止
或者在系统启动后，立刻登录系统执行
~~~
sudo systemctl disable power_cycle_check.service
