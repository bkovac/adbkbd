ifneq (${KERNELRELEASE},)
	obj-m = adb.o
	adb-objs = adb_lowlevel.o adb_hid.o adbkbd.o
else
	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
	MODULE_DIR := $(shell pwd)

.PHONY: all

all: modules

.PHONY:modules

modules:
	${MAKE} -C ${KERNEL_DIR} SUBDIRS=${MODULE_DIR}  modules

clean:
	rm -f *.o *.ko *.mod.c .*.o .*.ko .*.mod.c .*.cmd *~
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions
endif
