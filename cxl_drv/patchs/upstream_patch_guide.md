# Linux CXL 上游 Patch 投稿指南

## 一、问题在上游的确认状态

**结论：问题存在于当前 Linux mainline master 和 cxl/next 分支。**

经 2026-06-18 在线验证：

| 位置 | 有问题的代码 |
|------|-------------|
| `torvalds/linux` master (`drivers/cxl/pci.c` L697) | `if (!cxlds->reg_map.device_map.mbox.valid)` **存在** |
| `git.kernel.org/cxl/cxl.git` next 分支 (L1809) | `if (!cxlds->reg_map.device_map.mbox.valid)` **存在** |

---

## 二、Bug 分析（用于撰写 commit message）

### 根本原因

`cxl_pci_probe` 中的调用顺序如下：

```c
// Step 1: MEMDEV 寄存器枚举 → 写入【局部变量 map】
rc = cxl_pci_setup_regs(pdev, CXL_REGLOC_RBI_MEMDEV, &map);
if (rc)
    return rc;   // 内部 cxl_probe_regs() 若 mbox.valid==false 返回 -ENXIO

// Step 2: MEMDEV 寄存器 IO 映射，mbox IO 地址此时已就绪
rc = cxl_map_device_regs(&map, &cxlds->regs);
if (rc)
    return rc;

// Step 3: COMPONENT 寄存器枚举 → 写入 cxlds->reg_map
//   内部 __cxl_find_regblock_instance 用 compound literal 将 cxlds->reg_map 清零，
//   然后填充 component_map（union 导致与 device_map 重叠）
rc = cxl_pci_setup_regs(pdev, CXL_REGLOC_RBI_COMPONENT, &cxlds->reg_map);

// Step 4: 检查 cxlds->reg_map.device_map.mbox.valid
//   但 MEMDEV 信息只写入了局部 map，从未写入 cxlds->reg_map！
//   实际读到的是 component_map.ras.valid（union 重叠同一字节）
rc = cxl_pci_type3_init_mailbox(cxlds);
```

该检查同时存在两个问题：

1. **检查的对象是错误的 struct**：MEMDEV 信息写入局部 `map`，`cxlds->reg_map.device_map.mbox.valid` 从未被设置为 true
2. **逻辑上是死代码**：若 mbox 真正缺失，Step 1 中 `cxl_probe_regs()` 已返回 `-ENXIO`；能执行到 Step 4，说明 mbox 必然存在

**触发场景**：无 RAS capability 的 CXL 设备（如 DCD 设备），`component_map.ras.valid = false`，通过 union 重叠使 `device_map.mbox.valid` 读为 false，导致 `-ENODEV (-19)`。

---

## 三、Patch 草稿

### 3.1 Commit Message

```
cxl/pci: Remove incorrect mbox.valid check in cxl_pci_type3_init_mailbox()

The driver's design intent is that missing or malformed component
registers should not prevent mailbox initialization. cxl_pci_probe()
already reflects this: the CXL_REGLOC_RBI_COMPONENT setup path only
emits a dev_warn() and continues when component registers are absent,
rather than returning an error.

The check 'if (!cxlds->reg_map.device_map.mbox.valid)' violates this
intent and is also technically incorrect for two reasons:

1. Wrong struct: the MEMDEV register block is enumerated into a local
   variable 'map', not into 'cxlds->reg_map'. The device_map.mbox.valid
   field inside cxlds->reg_map is never written by the MEMDEV probe and
   will always read as zero regardless of actual hardware capability.

2. Already validated: cxl_pci_setup_regs(CXL_REGLOC_RBI_MEMDEV) calls
   cxl_probe_regs() which explicitly checks mbox.valid and returns
   -ENXIO if the mailbox is absent. If that check passes, the mailbox is
   guaranteed to be present by the time cxl_pci_type3_init_mailbox() is
   called.

The value that the check actually reads is component_map.ras.valid,
which aliases device_map.mbox.valid in the union. This is populated by
the COMPONENT probe, not the MEMDEV probe. On devices where the
component register BAR does not implement a CXL Component Capability
Array (e.g. certain DCD devices), cxl_probe_component_regs() returns
early leaving ras.valid=false. Through the union, this makes mbox.valid
read as false, causing cxl_pci_type3_init_mailbox() to return -ENODEV
(-19) even though the mailbox hardware is fully functional.

Remove the check. Mailbox presence has already been validated by
cxl_pci_setup_regs(CXL_REGLOC_RBI_MEMDEV). The presence or absence of
component registers is irrelevant to mailbox initialization.q

Fixes: <commit that introduced the check>
Signed-off-by: Your Name <your.email@example.com>
```

> **说明**：上面的 commit message 结构按照内核社区习惯，以**设计意图违反**为第一论点，
> 技术细节为佐证，最后一段是简洁的修复说明。这样维护者能在第一段就理解"为什么要改"，
> 而不需要先读完技术分析。

### 3.2 Diff

```diff
diff --git a/drivers/cxl/pci.c b/drivers/cxl/pci.c
index XXXXXXX..XXXXXXX 100644
--- a/drivers/cxl/pci.c
+++ b/drivers/cxl/pci.c
@@ -690,12 +690,6 @@ static int cxl_pci_type3_init_mailbox(struct cxl_dev_state *cxlds)
 {
 	int rc;
 
-	/*
-	 * Fail the init if there's no mailbox. For a type3 this is out of spec.
-	 */
-	if (!cxlds->reg_map.device_map.mbox.valid)
-		return -ENODEV;
-
 	rc = cxl_mailbox_init(&cxlds->cxl_mbox, cxlds->dev);
 	if (rc)
 		return rc;
```

> **注意**：正式提交前需通过 `git log --oneline drivers/cxl/pci.c` 找到引入该检查的原始 commit，填入 `Fixes:` 标签。

---

## 四、CXL 驱动维护信息（2026-06）

### 维护分支

```
开发树：git://git.kernel.org/pub/scm/linux/kernel/git/cxl/cxl.git
活跃分支：next（面向下一个合并窗口）
当前修复分支：for-7.2/cxl-pci-fixes（pci 相关 fix 首选投递目标）
```

### 维护者

```
Davidlohr Bueso      <dave@stgolabs.net>
Jonathan Cameron     <jic23@kernel.org>
Dave Jiang           <dave.jiang@intel.com>
Alison Schofield     <alison.schofield@intel.com>
Vishal Verma         <vishal.l.verma@intel.com>
Ira Weiny            <ira.weiny@intel.com>
Dan Williams         <djbw@kernel.org>
```

### 邮件列表

```
linux-cxl@vger.kernel.org（主列表，CC 必须）
linux-kernel@vger.kernel.org（LKML，可选 CC）
```

---

## 五、提交步骤

### 5.1 环境准备

```bash
# 克隆 CXL 开发树
git clone git://git.kernel.org/pub/scm/linux/kernel/git/cxl/cxl.git
cd cxl
git checkout -b fix/remove-redundant-mbox-valid-check origin/next

# 或基于 mainline 也可
git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
```

### 5.2 制作 Patch

```bash
# 修改文件
vim drivers/cxl/pci.c

# 找到引入该检查的原始 commit（用于 Fixes: 标签）
git log --oneline --follow -p drivers/cxl/pci.c | grep -B5 "mbox.valid"

# 验证 checkpatch
./scripts/checkpatch.pl --strict 0001-cxl-pci-Remove-redundant-mbox.valid-check.patch

# 查找需要抄送的维护者
./scripts/get_maintainer.pl 0001-cxl-pci-Remove-redundant-mbox.valid-check.patch

# 生成 patch 文件
git format-patch -1 --subject-prefix="PATCH" -o outgoing/
```

### 5.3 发送 Patch

```bash
#g 配置 git send-email（首次使用）
git config --global sendemail.smtpserver smtp.example.com
git config --global sendemail.smtpuser your.email@example.com

# 发送（--to 主维护者，--cc 列表和其他维护者）
git send-email \
    --to="Dave Jiang <dave.jiang@intel.com>" \
    --cc="linux-cxl@vger.kernel.org" \
    --cc="Davidlohr Bueso <dave@stgolabs.net>" \
    --cc="Jonathan Cameron <jic23@kernel.org>" \
    --cc="Dan Williams <djbw@kernel.org>" \
    outgoing/0001-*.patch
```

---

## 六、社区提交注意事项

### 6.1 格式规范（checkpatch 必须通过）

- **Subject 格式**：`[PATCH] cxl/pci: <动词开头的一句话描述>`
  - 子系统前缀 `cxl/pci:` 表示改动在 `drivers/cxl/pci.c`
  - 动词用祈使句：Remove / Fix / Add / Refactor
- **Signed-off-by**：必须有，使用真实姓名和常用邮箱
- **Fixes 标签**：若是 bug fix，必须添加 `Fixes: <12位SHA> ("<commit title>")`
- **行宽**：commit message body 每行不超过 75 字符

### 6.2 Commit Message 写法要点

```
<subject>: 不超过 72 字符，祈使句

第一段：用 1-2 句话说明"现象"和"为什么是 bug"。
        不要只说"this fixes a bug"，要说清楚 bug 的根本原因。

第二段（可选）：说明修复方案的合理性，以及为什么这样改。

Fixes: xxxxxxxxxxxx ("cxl/pci: ...")
Cc: stable@vger.kernel.org  ← 若影响已发布版本，加此行
Reported-by: Someone <someone@example.com>  ← 若有人报告
Signed-off-by: Your Name <your.email@example.com>
```

### 6.3 常见被拒原因

| 问题 | 说明 |
|------|------|
| checkpatch 报 WARNING/ERROR | 必须全部清零（至少 ERROR 清零）|
| 缺少 Fixes 标签 | bug fix 类 patch 必须有 |
| commit message 太简单 | "Remove dead code" 不够，要解释为何是死代码 |
| 没有 CC 邮件列表 | linux-cxl@vger.kernel.org 必须 CC |
| patch 依赖未说明 | 若基于非 mainline 的分支，需说明 base |
| 第一次 review 未及时回复 | 维护者 review 后需在 1-2 周内响应，否则会被遗忘 |

### 6.4 时间窗口

```
Linux 开发周期：约 9-10 周
  Week 1-2:    合并窗口（Merge Window）— 此时提 feature
  Week 3-10:   -rc1 到 -rc8 稳定期 — 此时只接受 bug fix

CXL 特定流程：
  patches → linux-cxl@vger.kernel.org
  → 维护者 review → 合入 cxl/cxl.git next 分支
  → Dave Jiang 在合并窗口期间发 pull request 给 Linus
```

**建议**：此 patch 属于 bug fix，可在任意时间提交（包括 -rc 期间），
并考虑加上 `Cc: stable@vger.kernel.org`，以便回溯修复到已发布的 LTS 内核。

### 6.5 提交前自查清单

```
[ ] ./scripts/checkpatch.pl --strict 无 ERROR，无 WARNING（或有合理解释）
[ ] ./scripts/get_maintainer.pl 输出的人员均已 CC
[ ] commit message 包含 Fixes: 标签（已找到引入 bug 的原始 commit）
[ ] Signed-off-by 使用真实姓名
[ ] git format-patch 生成的 patch 可被 git am 干净应用
[ ] 已在硬件上验证修复有效（或说明测试环境）
[ ] 搜索 lore.kernel.org 确认无重复提交
    搜索：https://lore.kernel.org/linux-cxl/?q=mbox.valid+cxl_pci_type3
```

### 6.6 搜索已有 Patch

在提交前，先在 CXL 邮件列表归档搜索是否已有人提交：

```
https://lore.kernel.org/linux-cxl/
关键词：mbox.valid  cxl_pci_type3_init_mailbox  component registers  -ENODEV
```

---

## 七、完整投稿模板（可直接使用）

以下是用 `git send-email` 发出的邮件正文参考：

```
From: Your Name <your.email@example.com>
To: Dave Jiang <dave.jiang@intel.com>
Cc: linux-cxl@vger.kernel.org,
    Davidlohr Bueso <dave@stgolabs.net>,
    Jonathan Cameron <jic23@kernel.org>,
    Dan Williams <djbw@kernel.org>
Subject: [PATCH] cxl/pci: Remove incorrect mbox.valid check in cxl_pci_type3_init_mailbox()

The driver's design intent is that missing or malformed component
registers should not prevent mailbox initialization. cxl_pci_probe()
already reflects this: the CXL_REGLOC_RBI_COMPONENT setup path only
emits a dev_warn() and continues when component registers are absent,
rather than returning an error.

The check 'if (!cxlds->reg_map.device_map.mbox.valid)' violates this
intent and is also technically incorrect for two reasons:

1. Wrong struct: the MEMDEV register block is enumerated into a local
   variable 'map', not into 'cxlds->reg_map'. The device_map.mbox.valid
   field inside cxlds->reg_map is never written by the MEMDEV probe and
   will always read as zero regardless of actual hardware capability.

2. Already validated: cxl_pci_setup_regs(CXL_REGLOC_RBI_MEMDEV) calls
   cxl_probe_regs() which explicitly checks mbox.valid and returns
   -ENXIO if the mailbox is absent. If that check passes, the mailbox is
   guaranteed to be present by the time cxl_pci_type3_init_mailbox() is
   called.

The value the check actually reads is component_map.ras.valid, which
aliases device_map.mbox.valid in the union. This is populated by the
COMPONENT probe, not the MEMDEV probe. On devices where the component
register BAR does not implement a CXL Component Capability Array (e.g.
certain DCD devices), cxl_probe_component_regs() returns early leaving
ras.valid=false. Through the union, this makes mbox.valid read as false,
causing cxl_pci_type3_init_mailbox() to return -ENODEV (-19) even
though the mailbox hardware is fully functional.

Remove the check. Mailbox presence has already been validated by
cxl_pci_setup_regs(CXL_REGLOC_RBI_MEMDEV). The presence or absence of
component registers is irrelevant to mailbox initialization.

Fixes: <commit SHA> ("<original commit title>")
Cc: stable@vger.kernel.org
Signed-off-by: Your Name <your.email@example.com>
---
 drivers/cxl/pci.c | 6 ------
 1 file changed, 6 deletions(-)

diff --git a/drivers/cxl/pci.c b/drivers/cxl/pci.c
index XXXXXXX..XXXXXXX 100644
--- a/drivers/cxl/pci.c
+++ b/drivers/cxl/pci.c
@@ -690,12 +690,6 @@ static int cxl_pci_type3_init_mailbox(struct cxl_dev_state *cxlds)
 {
 	int rc;
 
-	/*
-	 * Fail the init if there's no mailbox. For a type3 this is out of spec.
-	 */
-	if (!cxlds->reg_map.device_map.mbox.valid)
-		return -ENODEV;
-
 	rc = cxl_mailbox_init(&cxlds->cxl_mbox, cxlds->dev);
 	if (rc)
 		return rc;
-- 
2.x.x
```
