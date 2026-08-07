# patch_init_mb_fail.diff 说明

## 问题现象

CXL 设备（尤其是 DCD 类设备）在 probe 阶段报错：

```
cxl_pci_type3_init_mailbox failed: -19
```

`-19` 即 `-ENODEV`，来自 `cxl_pci_type3_init_mailbox` 中的如下检查：

```c
if (!cxlds->reg_map.device_map.mbox.valid)
    return -ENODEV;
```

## 根本原因：该检查是逻辑死代码

在 `cxl_pci_probe` 中，调用 `cxl_pci_type3_init_mailbox` 之前已经完成了两步：

```c
/* Step 1: 通过 Register Locator DVSEC 找到 RBI_MEMDEV 寄存器块，
 *         内部的 cxl_setup_regs() 会探测并验证 mbox/status/memdev，
 *         任一 valid == false 则直接返回 -ENXIO。
 */
rc = cxl_pci_setup_regs(pdev, CXL_REGLOC_RBI_MEMDEV, &map);
if (rc)
    return rc;   /* <-- mbox.valid == false 时在此已退出 */

/* Step 2: 将 mbox 等寄存器物理地址 iomap 到内核虚拟地址。
 *         执行到这里，mbox.valid 必然为 true。
 */
rc = cxl_map_device_regs(&map, &cxlds->regs.device_regs);
if (rc)
    return rc;

/* ... */

/* Step 3: 能走到这里，mbox.valid 一定已经通过了 Step 1 的检查。
 *         因此再次检查 mbox.valid 是逻辑死代码。
 */
rc = cxl_pci_type3_init_mailbox(cxlds);
```

`cxl_setup_regs`（由 `cxl_pci_setup_regs` 内部调用）对 `RBI_MEMDEV` 类型的验证：

```c
/* core/regs.c */
case CXL_REGLOC_RBI_MEMDEV:
    cxl_probe_device_regs(host, base, dev_map);
    if (!dev_map->status.valid || !dev_map->mbox.valid ||
        !dev_map->memdev.valid) {
        dev_err(host, "registers not found: %s%s%s\n", ...);
        return -ENXIO;
    }
```

**结论**：`cxl_pci_type3_init_mailbox` 中的 `mbox.valid` 检查是冗余的死代码。
程序能到达该函数，就意味着 `mbox.valid` 在 Step 1 中已验证为 true。

## 额外背景：union 重叠 Bug

上游内核（含 v6.17）中，`cxl_register_map` 使用 union：

```c
union {
    struct cxl_component_reg_map component_map;
    struct cxl_device_reg_map    device_map;
    struct cxl_pmu_reg_map       pmu_map;
};
```

`component_map.ras.valid` 与 `device_map.mbox.valid` 在内存中重叠在同一字节。
`cxl_pci_probe` 的执行顺序是先 MEMDEV probe、再 COMPONENT probe，COMPONENT probe
会将 `ras.valid` 写入该字节，覆盖之前 MEMDEV probe 写入的 `mbox.valid`。

对于没有 RAS capability 的 DCD 设备：`ras.valid = false` → 读到的 `mbox.valid = false`
→ `cxl_pci_type3_init_mailbox` 返回 `-ENODEV`。

这个 union 重叠 bug 是触发问题的直接原因，但即便没有此 bug，该检查也是冗余的（见上文分析）。

## 修复方案

直接删除 `cxl_pci_type3_init_mailbox` 中冗余的 `mbox.valid` 检查：

```diff
-	/*
-	 * Fail the init if there's no mailbox. For a type3 this is out of spec.
-	 */
-	if (!cxlds->reg_map.device_map.mbox.valid)
-		return -ENODEV;
-
```

**改动范围**：仅 `pci.c`，删除 6 行，不触碰任何数据结构或其他模块。

## 对比：Jurian patch 链（patch1+patch2+patch3）

Jurian 的方案从根因出发，修复 union 重叠问题：
- `patch1`：`cxl.h` 中 union → struct（anonymous struct 包裹）
- `patch2`：`core/regs.c` 用 `#if 0` 禁用 compound literal 初始化，防止 COMPONENT probe 清零 `device_map`
- `patch3`：`pci.c` PMU 改用局部变量，避免污染 `cxlds->reg_map.resource`

| 方案 | 改动文件数 | 改动行数 | 备注 |
|------|-----------|---------|------|
| `patch_init_mb_fail.diff` | 1（pci.c） | 6 行（删除） | 推荐，最小侵入 |
| Jurian patch1+2+3 | 3（cxl.h / regs.c / pci.c） | ~50 行 | 修改核心数据结构，含 `#if 0` hack |

对于 out-of-tree 驱动，修改 `cxl.h` 中的 union 会影响 ABI，风险远高于直接删除冗余检查。
`patch_init_mb_fail.diff` 是更合适的修复方式。
