# Check $ARCH and $CROSS_COMPILE before running make
# CROSS_COMPILE should not be set for native compiler
#
#
ifneq ($(KERNELRELEASE),)

obj-m := ir-dev.o

else
KDIR  := /lib/modules/4.4.22-v7+/build
PWD   := $(shell pwd)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm *.mod.c *.o .*.cmd Module.symvers modules.order || :
	-rm -rf .tmp_versions || :

.PHONY: clean

endif
