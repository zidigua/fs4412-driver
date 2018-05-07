MYMOD ?= fifo

PWD = $(shell pwd)  
KERNEL_SRC = /home/sky/share/linux-3.14
   
obj-m := $(MYMOD).o  
   
all:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  
	cp $(MYMOD).ko /rootfs/home/ 
  
         
clean:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
