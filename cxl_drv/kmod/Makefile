# SPDX-License-Identifier: GPL-2.0
#
# Standalone build wrapper for CXL kernel modules.
#
# Usage (from this directory):
#   make modules              # build cxl_core.ko and cxl_pci.ko
#   make cxl_core.ko          # build cxl_core.ko only
#   make cxl_pci.ko           # build cxl_pci.ko only
#   make modules_install      # install built modules
#   make clean
#
# Override kernel build tree if needed:
#   make KERNEL_SRC=/path/to/kernel/build modules

ifneq ($(KERNELRELEASE),)
# kbuild part

ccflags-y += -I$(srctree)/drivers/cxl -I../../../include/cxl/
CFLAGS_trace.o = -DTRACE_INCLUDE_PATH=$(srctree)/drivers/cxl/core \
		 -I$(srctree)/drivers/cxl/core

obj-m += cxl_core.o
obj-m += cxl_pci.o

cxl_core-y := ../core/port.o
cxl_core-y += ../core/pmem.o
cxl_core-y += ../core/regs.o
cxl_core-y += ../core/memdev.o
cxl_core-y += ../core/mbox.o
cxl_core-y += ../core/pci.o
cxl_core-y += ../core/hdm.o
cxl_core-y += ../core/pmu.o
cxl_core-y += ../core/cdat.o
cxl_core-y += ../core/ras.o
cxl_core-$(CONFIG_TRACING) += ../core/trace.o
cxl_core-$(CONFIG_CXL_REGION) += ../core/region.o
cxl_core-$(CONFIG_CXL_MCE) += ../core/mce.o
cxl_core-$(CONFIG_CXL_FEATURES) += ../core/features.o
cxl_core-$(CONFIG_CXL_EDAC_MEM_FEATURES) += ../core/edac.o

cxl_pci-y := ../pci.o

else
# normal makefile

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

.PHONY: all modules modules_install clean cxl_core.ko cxl_pci.ko

all: modules

modules cxl_core.ko cxl_pci.ko:
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) $(if $(filter modules,$(MAKECMDGOALS)),modules,$@)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) clean

endif
