MYMOD ?= fifo

PWD = $(shell pwd)  
KERNEL_SRC = /home/sky/share/linux-3.14
   
obj-m := $(MYMOD).o  
   
all:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  
	arm-linux-gcc mycat_mmap.c -o mycat_mmap
	arm-linux-gcc fifo_write.c -o fifo_write
	cp $(MYMOD).ko  fifo_write mycat_mmap /rootfs/home/
  
         
clean:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
