# Modify the Makefile to point to Linux build tree
DIST ?= $(shell uname -r)
KDIR := /lib/modules/$(DIST)/build/

# Include your kernel configuration
include /boot/config-$(DIST)

MODULE_NAME=doe
$(MODULE_NAME)-objs += doe_main.o libdoe/pcie-doe.o
obj-m += $(MODULE_NAME).o

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
