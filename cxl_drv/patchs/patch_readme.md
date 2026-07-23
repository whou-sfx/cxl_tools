# CXL Driver Patch 说明

本文档记录 `cxl_drv` out-of-tree 模块的编译问题修复，以及 Jurian 三个 patch 的设计意图与 Bug 分析。

---

## 一、`cxl_pci_type3_init_mailbox failed: -19` 根因分析

### 上游"巧合机制"

上游 Linux 内核（含 v6.17）的 `cxl_register_map` 结构使用 **union**：

```c
struct cxl_register_map {
    ...
    union {
        struct cxl_component_reg_map component_map;
        struct cxl_device_reg_map    device_map;
        struct cxl_pmu_reg_map       pmu_map;
    };
};
```

三个子 map 共享同一块内存。内存布局对比：

```
component_map: [ hdm_decoder(cxl_reg_map) | ras(cxl_reg_map)  ]
device_map:    [ status(cxl_reg_map)      | mbox(cxl_reg_map) | memdev(cxl_reg_map) ]
               ↑ 同一起始地址 ──────────────────────────────────↑
               component_map.ras.valid  与  device_map.mbox.valid  共享同一内存字节！
```

上游 `cxl_pci_probe` 的寄存器设置顺序：

1. MEMDEV probe → 写入**局部变量** `map`（`map.device_map.mbox.valid = true`）
2. COMPONENT probe → 写入 `cxlds->reg_map`（`reg_map.component_map.ras.valid = true/false`）
3. `cxl_pci_type3_init_mailbox` 检查 `cxlds->reg_map.device_map.mbox.valid`

由于 union，步骤 3 实际读到的是 `component_map.ras.valid`（与 `device_map.mbox.valid` 重叠）。

**巧合行为**：只要设备有 RAS capability，`ras.valid = true`，检查就能通过。

**失效场景**：DCD 等设备没有 RAS capability，`ras.valid = false`
→ `device_map.mbox.valid = false` → `cxl_pci_type3_init_mailbox` 返回 `-ENODEV (-19)`。

---

## 二、Patch 链说明

### patch1.diff — Jurian 第一版

**变更内容**：
- `cxl.h`：将 `union { ... }` 改为 `struct { ... }`（anonymous struct），三个子 map 不再重叠
- `pci.c`：MEMDEV probe 改用 `cxlds->reg_map`（原来用局部 `map`），显式填充 `device_map`

**设计意图**：MEMDEV probe 的结果保存在 `cxlds->reg_map.device_map`，`cxl_pci_type3_init_mailbox`
可以直接读到真实的 `mbox.valid`，不再依赖 union 重叠的巧合。

**遗留 Bug**：`__cxl_find_regblock_instance` 函数开头的 compound literal 初始化会清零整个 struct：

```c
*map = (struct cxl_register_map) {
    .host = &pdev->dev,
    .resource = CXL_RESOURCE_NONE,
};  // 等价于先将整个 struct 置零，再赋两个字段
```

COMPONENT probe 调用此函数时，`map = &cxlds->reg_map`，导致之前 MEMDEV probe 写入的
`device_map.mbox.valid = true` 被**清零为 false**。由于 union 已去掉（字段不重叠），
COMPONENT probe 只填 `component_map`，`device_map.mbox.valid` 仍为 false。

结果：`cxl_pci_type3_init_mailbox` 依然返回 `-ENODEV`。

### patch2.diff — Jurian 第二版

**变更内容**：
- `core/regs.c`：用 `#if 0` 禁用 compound literal 初始化，改为只重置两个必要字段：
  ```c
  #if 0
      *map = (struct cxl_register_map) { .host = ..., .resource = ... };
  #else
      map->host = &pdev->dev;
      map->resource = CXL_RESOURCE_NONE;
  #endif
  ```
- `cxl.h`：去掉 anonymous struct 包装层，三个子 map 作为直接成员

**修复逻辑**：COMPONENT probe 调用 `__cxl_find_regblock_instance` 时，只重置 `host`
和 `resource`，不清零 `device_map`。MEMDEV probe 写入的 `device_map.mbox.valid = true` 得以保留。

`cxl_probe_component_regs` 内部会自行清零 `component_map`（`*map = (struct cxl_component_reg_map){ 0 }`），
两个子 map 独立管理，互不影响。

### patch3.diff — Jurian 第三版（**已合并入当前代码**）

**变更内容**：
- `pci.c`：PMU 枚举循环改用局部变量 `pmu_map`，不再复用 `cxlds->reg_map`

**修复逻辑**：patch1+patch2 将 COMPONENT 的 BAR 地址写入 `cxlds->reg_map.resource`。
若 PMU 也写入 `cxlds->reg_map`，则 `resource` 会被再次覆盖为 PMU BAR 地址，
破坏后续可能用到 `cxlds->reg_map.resource` 的逻辑。改用局部 `pmu_map` 可隔离副作用。

```c
// 修复后（当前代码）
for (i = 0; i < pmu_count; i++) {
    struct cxl_pmu_regs pmu_regs;
    struct cxl_register_map pmu_map;   // 局部变量，不污染 cxlds->reg_map

    rc = cxl_find_regblock_instance(pdev, CXL_REGLOC_RBI_PMU, &pmu_map, i);
    ...
    rc = cxl_map_pmu_regs(&pmu_map, &pmu_regs);
    ...
}
```

### 当前代码状态

| patch | 内容 | 状态 |
|-------|------|------|
| patch1 | union→struct，MEMDEV 用 cxlds->reg_map | 已合并 |
| patch2 | #if 0 禁用 compound literal 清零 | 已合并 |
| patch3 | PMU 用局部 pmu_map | 已合并 |


---

## 三、编译错误修复（Makefile）

### 问题现象

远端 make 时报错：

```
fatal error: trace/events/trace.h: No such file or directory
  107 | # define __TRACE_INCLUDE(system) <trace/events/system.h>
compilation terminated.
make: *** [Makefile:88: modules] Error 2
```

### 根本原因

kbuild 通过 `scripts/Makefile.lib` 中的 `target-stem` 变量来查找 per-file 编译标志：

```makefile
# scripts/Makefile.lib
target-stem = $(basename $(patsubst $(obj)/%,%,$@))
_c_flags = ... $(CFLAGS_$(target-stem).o)
```

out-of-tree 构建时，`trace.c` 的编译目标路径为 `$(obj)/core/trace.o`，所以：

```
target-stem = core/trace
kbuild 实际查找的变量 = CFLAGS_core/trace.o
```

但 `Makefile` 中写的是 `CFLAGS_trace.o`，与 `core/trace` 不匹配，导致
`-DTRACE_INCLUDE_PATH` 未传给编译器，`define_trace.h` 回落到默认路径
`<trace/events/trace.h>`，文件不存在，编译失败。

对比：上游 in-tree `drivers/cxl/core/Makefile` 中写的是 `CFLAGS_trace.o`，
因为 in-tree 构建时源文件就在 `core/` 目录下，`target-stem` 为 `trace`，能匹配。

### 修复方法

将 `Makefile` out-of-tree 段中的变量名改为包含子目录前缀：

```makefile
# 修改前
CFLAGS_trace.o = -DTRACE_INCLUDE_PATH=$(src)/core -I$(src)/core

# 修改后
CFLAGS_core/trace.o = -DTRACE_INCLUDE_PATH=$(src)/core -I$(src)/core
```


---

## 四、commit 7de96287 为何未生效

commit `7de96287`（"merge component reg map fail issue from jurian's dcd branch"）
已将 patch1+patch2+patch3 的逻辑合并入代码，逻辑上正确。

**实际失败原因**：由于 Makefile 存在 `CFLAGS_trace.o` 变量名错误，远端 `make` 编译
失败（`trace/events/trace.h: No such file or directory`），模块未能重新编译，
远端加载的仍是旧 `.ko` 文件。

旧 `.ko` 可能处于 patch1 已应用、但 patch2 的 `#if 0` 尚未生效的中间状态：

- union 已去掉（字段不重叠）✓
- MEMDEV 已改用 `cxlds->reg_map` ✓
- `__cxl_find_regblock_instance` 的 compound literal **仍在清零** `device_map.mbox.valid` ✗

COMPONENT probe 将 `device_map.mbox.valid` 清零，`cxl_pci_type3_init_mailbox` 返回 `-ENODEV`。

---

## 五、推荐修复步骤

```bash
# 1. 确认 Makefile 已修复（CFLAGS_core/trace.o）
grep "CFLAGS.*trace" Makefile

# 2. 完整重编译
make clean && make

# 3. 安装并重新加载模块
sudo make modules_install
sudo modprobe -r cxl_pci cxl_mem cxl_pmem cxl_port cxl_acpi cxl_core
sudo modprobe cxl_core cxl_port cxl_acpi cxl_pmem cxl_mem cxl_pci
```
