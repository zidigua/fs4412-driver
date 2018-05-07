
KERNELDIR := /home/sky/share/linux-3.14/
PWD := $(shell pwd)

obj-m := key.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	arm-linux-gcc app_key.c -o app_key
	cp *.ko app_key /rootfs/home/

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

