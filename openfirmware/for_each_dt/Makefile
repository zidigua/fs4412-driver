
LINUXDIR := /home/sky/share/linux-3.14/ 
DIR := $(shell pwd)

obj-m := for_each_dt.o


drfault:
	$(MAKE) -C $(LINUXDIR) M=$(DIR) modules

clean:
	$(MAKE) -C $(LINUXDIR) M=$(DIR) clean

