ROOTFS_DIR=/rootfs

MODULE_NAME=mpu6050_i2c_drv
APP_NAME=simple_input_test

CROSS_COMPILE= /opt/gcc-4.6.4/bin/arm-linux-
CC=$(CROSS_COMPILE)gcc

ifeq ($(KERNELRELEASE),)
KERNEL_DIR=/home/sky/share/linux-3.14/
CUR_DIR=$(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(CUR_DIR) modules
	$(CC) $(APP_NAME).c -o $(APP_NAME)
clean:
	make -C $(KERNEL_DIR) M=$(CUR_DIR) clean
	rm -rf $(APP_NAME) 
install:
	cp -raf *.ko $(APP_NAME)  $(ROOTFS_DIR)/home
else
	obj-m+=$(MODULE_NAME).o
endif
