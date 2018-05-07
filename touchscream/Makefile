
KERNEL_DIR:=/home/sky/share/linux-3.14
#KERNEL_DIR:=/lib/modules/$(shell uname -r)/build
PWD       :=$(shell pwd)

APP 	   =test
obj-m += ts-gt818.o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
	arm-linux-gcc $(APP).c -o $(APP)
install:
	cp *.ko $(APP) ~/source/rootfs/ 
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -rf $(APP)


