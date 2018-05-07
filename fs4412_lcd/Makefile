
ifneq ($(KERNELRELEASE),)
obj-m = fs4412_fimd.o

else

KERNELDIR := /home/sky/share/linux-3.0-fs4412_V3/
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp fs4412_fimd.ko /home/nfsdir -a

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

endif



