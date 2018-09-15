
LINUXDIR := /home/sky/share/kernel/linux-3.14.25/
PWD := $(shell pwd)

obj-m := adc.o


default:
	$(MAKE) -C $(LINUXDIR) M=$(PWD) modules
	arm-linux-gcc -o app_adc app_adc.c -pthread
	cp *.ko app_adc /rootfs/home/
	
	
clean:
	$(MAKE) -C $(LINUXDIR) M=$(PWD) clean





