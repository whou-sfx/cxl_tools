# Modify the Makefile to point to Linux build tree
#DIST = /home/jurian/code/kernel/UBUNTU/5.15.60+/jammy.jurian
#DIST = /home/synopsys/jurian/jammy.orig
#DIST = /home/tcn/whou/tmp/jammy
#KDIR := $(DIST)
DIST ?= $(shell uname -r)
#KDIR := /lib/modules/$(DIST)/build/
KDIR := /home/jurian/code/kernel/UBUNTU/5.15.60+/jammy.jurian/
CONF = /home/jurian/code/kernel/UBUNTU/5.15.60+/jammy.jurian/config-5.15.60+

# Include your kernel configuration
#include /boot/config-5.15.60+
#include $(DIST)/config-5.15.60+
#include /boot/config-$(DIST)
include $(CONF)
CONFIG_MODULE_SIG=n

MODULE_NAME=doe
$(MODULE_NAME)-objs += doe_main.o libdoe/pcie-doe.o
obj-m += $(MODULE_NAME).o

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
#doe-y += doe_main.o
