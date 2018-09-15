

MODULE_NAME = fs4412_ds18b20

ifeq ($(KERNELRELEASE), )

KERNEL_DIR = /home/sky/linux-3.14-git/
CUR_DIR = $(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(CUR_DIR) modules

clean:
	make -C $(KERNEL_DIR) M=$(CUR_DIR) clean

install:
	cp -raf *.ko  /rootfs/home/

else

obj-m += $(MODULE_NAME).o

endif
