
PWD = $(shell pwd)  
KERNEL_SRC = /home/sky/share/linux-3.14/  
obj-m :=  main.o  test.o

all:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules  
	cp *.ko /rootfs/home/module/

			  
			           
clean:  
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean 
			
