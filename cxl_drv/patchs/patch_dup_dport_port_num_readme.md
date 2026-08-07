# cxl_mem probe -6 (ENXIO): duplicate PCIe "Port Number" under a CXL host bridge

## 症状

`cxl_mem` probe 失败，返回 `-6` (`-ENXIO`)：

```
[2875] cxl_mem mem0: scan: iter: mem0 dport_dev: 0000:89:01.2 parent: pci0000:89
[2875] cxl_mem mem0: 0000:89:01.2 is a root dport
[2875] cxl_mem mem0: probe: -6
```

在这之前，代表 bus 89 这个 CXL host bridge 的 `cxl_port`（`port6`）自己的 probe 先失败了：

```
cxl_port port6: unable to add dport247-0000:89:01.3 non-unique port id (0000:89:01.1)
pcieport 0000:89:01.3: failed to add dport to port6: -16
cxl_port port6: probe: -16
cxl_port port6: probe with driver cxl_port failed with error -16
```

`lspci -t` 显示 `mem0`（ScaleFlux 控制器）实际挂在 `0000:89:01.2` 下面，跟报错的
`0000:89:01.1` / `0000:89:01.3` 没有直接关系：

```
+-[0000:89]-+-00.0  ... Root Complex
|           +-00.3  ... RCEC
|           +-01.1-[8a-8b]--
|           +-01.2-[8c-8d]----00.0  ScaleFlux Inc. Device 1030
|           +-01.3-[8e-8f]--
|           +-01.4-[90-91]--
```

## 根因

1. `port6` 覆盖整条 bus 89，它的 `cxl_switch_port_probe()` 会一次性通过
   `devm_cxl_port_enumerate_dports()` (`drivers/cxl/core/pci.c:77`) 枚举 bus 89 下所有
   Root Port（`01.1`/`01.2`/`01.3`/`01.4`）作为自己的 dport。

2. 每个 dport 的 `port_id` 取自该 Root Port PCIe Link Capabilities 寄存器的
   **Port Number** 字段（`PCI_EXP_LNKCAP_PN`，`drivers/cxl/core/pci.c:59`）：
   ```c
   port_num = FIELD_GET(PCI_EXP_LNKCAP_PN, lnkcap);
   dport = devm_cxl_add_dport(port, &pdev->dev, port_num, map.resource);
   ```
   这台 AMD 平台的 BIOS/AGESA 给 `0000:89:01.1` 和 `0000:89:01.3` 这两个 Root Port
   编了**同一个 Port Number**（247）。PCIe spec 里这个字段只是"建议由厂商保证在同一
   root complex 内唯一"，并没有硬件强制校验——这是平台固件的一个 quirk。

3. `add_dport()` (`drivers/cxl/core/port.c:1051`) 发现 `port_id` 重复会拒绝并返回
   `-EBUSY`；`match_add_dports()` 把这个错误原样 `return`，导致
   `pci_walk_bus()` **立即停止遍历**（`01.4` 根本没被尝试到），并把错误一路带出
   `devm_cxl_port_enumerate_dports()`。

4. `cxl_switch_port_probe()` (`drivers/cxl/port.c:68-70`) 看到负返回值就让 `port6`
   整个 probe 失败。Linux 驱动核心对失败的 probe 会执行
   `devres_release_all(&port6->dev)`，把这次 probe 期间通过 devm 加进去的**所有**
   dport 一起释放——包括在遍历顺序上排在 `01.3` **之前**、已经成功加入且完全没有
   问题的 `01.1`、`01.2`。

5. 结果：`mem0` 在 `devm_cxl_enumerate_ports()` 里找不到任何已注册的 `cxl_port`
   拥有 `0000:89:01.2` 这个 dport（因为在第 4 步被连带清空了），只能把它当成
   "到达拓扑根都没找到 CXL root"，返回 `-ENXIO`。

**一句话**：`01.1` 和 `01.3` 之间硬件/固件层面的 Port Number 冲突，通过
"一个 dport 加失败就让整个 switch port probe 失败、进而回滚所有兄弟 dport"这个
过强的错误处理策略，连带拖垮了本来完全正常的 `01.2`（`mem0` 真正所在的口）。

## 上游现状

上游已经用另一套架构解决了同类问题（`4f06d81e7c6a` "cxl: Defer dport allocation
for switch ports"，已进入 v6.19 开发周期）：不再在 switch port probe 时一次性
枚举整条 bus 的所有 dport，而是推迟到每个 memdev 真正出现时才按需添加它自己那一个
dport，天然把故障范围限制在冲突口自身，不会影响其它兄弟口。这个改动改动面较大
（涉及 port 创建/decoder target 更新的整体流程），本 patch 不做 backport，而是
针对当前分支代码结构做一个最小侵入的 workaround。

## Workaround（`patch_dup_dport_port_num.diff`）

对 `drivers/cxl/core/pci.c` 做两处改动：

1. `match_add_dports()`：某个 dport 因 `port_id` 重复而添加失败时，记录错误后
   `return 0` 而不是把错误返回值传给 `pci_walk_bus()`，让遍历**继续处理后面的
   Root Port**（不再因为一个口撞车就让 `01.4` 也没机会被枚举）。同时打印一条
   `dev_warn`，明确告知管理员这个口被跳过、原因是什么、以及这只是一个
   workaround：
   ```
   cxl: skipping dport 0000:89:01.3 (duplicate PCIe Link Capabilities Port
   Number 247, err -16). Devices behind this port will not work. This is a
   kernel workaround for a platform BIOS quirk - apply
   patch_dup_dport_port_num.diff on kernels < 6.19, or update to kernel >=
   6.19 for the upstream fix (defer-dport-allocation) instead of relying on
   this workaround.
   ```
   这条日志级别是 `dev_warn`（默认可见，不需要开 dyndbg），目的是避免这个
   workaround 长期"隐形"生效——巡检 `dmesg`/日志采集系统时能直接看到提示，
   知道这台机器：(a) 有一个口因为固件 Port Number 冲突而失效，(b) 现在跑的
   是打了 workaround 补丁的内核，后续应该跟进平台固件修复或升级到 >= 6.19。
2. `devm_cxl_port_enumerate_dports()`：不再因为 `ctx.error` 非空就让整个函数
   返回错误，只要至少有一个 dport 成功入表（`ctx.count > 0`）就正常返回
   `ctx.count`，让 `port6` 的 probe 视为成功，避免 devm 回滚已经成功加入的
   健康 dport。

### 效果

| Root Port | 结果 |
|---|---|
| `89:01.1` | 正常注册（遍历顺序最先，拿到 `port_id=247`） |
| `89:01.2`（`mem0`） | 正常注册，`mem0` probe 恢复正常 |
| `89:01.3` | 仍然无法注册 dport（`port_id` 撞车，属于"先来后到"里的失败方） |
| `89:01.4` | 正常注册（不再因为 `01.3` 失败而被提前中止遍历） |

在这台机器当前的拓扑下，`8e-8f`（`01.3`）底下没有插任何 CXL 设备，所以这个改动
没有引入新的功能损失；如果以后有设备插到 `01.3` 下面，会复现和本次 `mem0`
相同的 `-ENXIO`（只是换了一个口），需要平台侧修正 BIOS 对 Port Number 字段的
赋值，或后续 backport 上游的 defer-dport 方案来根治。

## 验证方法

```bash
sudo lspci -vv -s 89:01.1 | grep "LnkCap:"
sudo lspci -vv -s 89:01.2 | grep "LnkCap:"
sudo lspci -vv -s 89:01.3 | grep "LnkCap:"
sudo lspci -vv -s 89:01.4 | grep "LnkCap:"
```
确认 `01.1`/`01.3` 的 `Port #` 相同、`01.2` 唯一，即可复现根因判断。

打上 patch 重新编译加载后，检查：
```
dmesg | grep -E "cxl_port port.*probe:|cxl_mem mem0: probe:|cxl: skipping dport"
```
`port6` 的 `probe:` 应变为非负数，`mem0` 的 `probe:` 应不再是 `-6`，同时应该能看到
一条 `cxl: skipping dport 0000:89:01.3 ...` 的 `dev_warn`，提示这个 workaround
被触发了，需要跟进平台固件修复或内核升级。
