
KERNELDIR := /home/sky/linux-3.14-git/
DIR := $(shell pwd)

obj-m := pwm.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(DIR) modules
	arm-linux-gcc app_pwm.c -o app_pwm 
	cp app_pwm *.ko /rootfs/home/

clean:
	$(MAKE) -C $(KERNELDIR) M=$(DIR) clean
	rm app_pwm

