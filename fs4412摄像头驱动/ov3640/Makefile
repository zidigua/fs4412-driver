ifneq ($(KERNELRELEASE),)
	obj-m := ov3640.o
else
	KERNELDIR := /home/linux/kernel/linux-3.14/
	PWD       := $(shell pwd)

modules:
	make -C $(KERNELDIR) M=$(PWD) modules
	cp *.ko ~/rootfs/
endif

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
