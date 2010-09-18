default: ksmews
KERNELDIR := /lib/modules/`uname -r`/build
PWD := `pwd`/
obj-m := ksmews.o

include $(M)ksmews-objs-list

EXTRA_CFLAGS += -DCHUNCKS_NBITS=5 -I$(M)/core -I$(M)/drivers -I$(M)/.

.PHONY: clean

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules clean
	$(RM) -rf Module.markers modules.order

ksmews:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
