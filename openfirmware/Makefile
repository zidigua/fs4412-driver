

SUBDIRS=driver device
.PHONY:all $(SUBDIRS)
all:$(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@
clean:
	find -name '*.ko *.mod.c *.o .*.ko.cmd .*.o.cmd .tmp_ver* ' | xargs rm -rf

