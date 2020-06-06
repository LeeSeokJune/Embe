obj-m 	:=module.o

KDIR	:=/work/achroimx_kernel
PWD	:=$(shell pwd)

all: driver
#all: driver

driver:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=arm


install:
	cp -a dev_driver.ko /nfsroot

clear:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.o
	rm -rf Module.symvers
	rm -rf modules.order
	rm -rf .tmp*
	rm -rf .dev_driver*
