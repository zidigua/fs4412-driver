

KERNELDIR := /home/sky/share/linux-3.14/
PWD := $(shell pwd)

obj-m := platform_driver.o
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp *.ko /rootfs/home/


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
