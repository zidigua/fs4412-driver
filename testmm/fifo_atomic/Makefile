MYMOD ?= fifo

PWD = $(shell pwd)  
KERNEL_SRC = /home/sky/share/linux-3.14
   
obj-m := $(MYMOD).o  
   
all:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  
	cp $(MYMOD).ko /rootfs/home/ 
  
         
clean:  
	rm *.ko  
	rm *.o 
	rm  module.ko  module.mod.c  module.mod.o  module.o  modules.order  Module.symvers
