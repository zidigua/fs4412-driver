
KERNELDIR := /home/sky/share/linux-3.14/
PWD := $(shell pwd)

obj-m := fifo.o
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	arm-linux-gcc fifo_app.c -o fifo_app
	arm-linux-gcc mycat_mmap.c -o mycat
	cp *.ko fifo_app mycat /rootfs/home/


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

