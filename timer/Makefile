
KERNELDIR := /lib/modules/$(shell uname -r)/build
SRCDIR := $(shell pwd)

obj-m := timer.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(SRCDIR) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(SRCDIR) clean




