
KERNELDIR := /home/sky/share/linux-3.14/
PWD := $(shell pwd)

obj-m := testmm.o
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
#	arm-linux-gcc app_mmap.c -o app_mmap
	cp *.ko  /rootfs/home/


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

