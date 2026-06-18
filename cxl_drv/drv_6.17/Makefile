# SPDX-License-Identifier: GPL-2.0
#
# Standalone build (from this directory):
#   make modules              # build all CXL modules
#   make cxl_core.ko          # build cxl_core.ko only
#   make cxl_pci.ko           # build cxl_pci.ko only
#   make cxl_port.ko          # build cxl_port.ko only
#   make cxl_acpi.ko          # build cxl_acpi.ko only
#   make cxl_pmem.ko          # build cxl_pmem.ko only
#   make cxl_mem.ko           # build cxl_mem.ko only
#   make modules_install      # install built modules
#   make clean
#
# Override kernel build tree if needed:
#   make KERNEL_SRC=/path/to/kernel/build modules
#
# Enable dev_dbg after insmod (requires CONFIG_DYNAMIC_DEBUG=y):
#   echo 'file drivers/cxl/* +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
# Or by module name (works even without prefix-map):
#   echo 'module cxl_pci +p' | sudo tee /sys/kernel/debug/dynamic_debug/control

ifneq ($(KERNELRELEASE),)

ifeq ($(M),)
# In-tree build
# Order is important here for the built-in case:
# - 'core' first for fundamental init
# - 'port' before platform root drivers like 'acpi' so that CXL-root ports
#   are immediately enabled
# - 'mem' and 'pmem' before endpoint drivers so that memdevs are
#   immediately enabled
# - 'pci' last, also mirrors the hardware enumeration hierarchy
obj-y += core/
obj-$(CONFIG_CXL_PORT) += cxl_port.o
obj-$(CONFIG_CXL_ACPI) += cxl_acpi.o
obj-$(CONFIG_CXL_PMEM) += cxl_pmem.o
obj-$(CONFIG_CXL_MEM) += cxl_mem.o
obj-$(CONFIG_CXL_PCI) += cxl_pci.o

cxl_port-y := port.o
cxl_acpi-y := acpi.o
cxl_pmem-y := pmem.o security.o
cxl_mem-y := mem.o
cxl_pci-y := pci.o

else
# Out-of-tree standalone build
#
# Remap __FILE__ to drivers/cxl/... so dynamic_debug works with:
#   echo 'file drivers/cxl/* +p' > /sys/kernel/debug/dynamic_debug/control

ccflags-y += -I$(src)
ccflags-y += -fmacro-prefix-map=$(abspath $(src))=drivers/cxl
CFLAGS_core/trace.o = -DTRACE_INCLUDE_PATH=$(src)/core -I$(src)/core


obj-m += cxl_core.o
obj-m += cxl_port.o
obj-m += cxl_acpi.o
obj-m += cxl_pmem.o
obj-m += cxl_mem.o
obj-m += cxl_pci.o

cxl_core-y := core/port.o
cxl_core-y += core/pmem.o
cxl_core-y += core/regs.o
cxl_core-y += core/memdev.o
cxl_core-y += core/mbox.o
cxl_core-y += core/pci.o
cxl_core-y += core/hdm.o
cxl_core-y += core/pmu.o
cxl_core-y += core/cdat.o
cxl_core-y += core/ras.o
cxl_core-$(CONFIG_TRACING) += core/trace.o
cxl_core-$(CONFIG_CXL_REGION) += core/region.o
cxl_core-$(CONFIG_CXL_MCE) += core/mce.o
cxl_core-$(CONFIG_CXL_FEATURES) += core/features.o
cxl_core-$(CONFIG_CXL_EDAC_MEM_FEATURES) += core/edac.o

cxl_port-y := port.o
cxl_acpi-y := acpi.o
cxl_pmem-y := pmem.o security.o
cxl_mem-y := mem.o
cxl_pci-y := pci.o

endif

else

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
MODULES := cxl_core.ko cxl_port.ko cxl_acpi.ko cxl_pmem.ko cxl_mem.ko cxl_pci.ko

.PHONY: all modules modules_install clean $(MODULES)

all: modules

modules $(MODULES):
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) $(if $(filter modules,$(MAKECMDGOALS)),modules,$@)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) clean

endif
