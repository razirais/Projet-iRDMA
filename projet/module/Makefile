# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
    	obj-m := rocketIO.o rocketIO_ao.o
    	# Otherwise we were called directly from the command
	#line; invoke the kernel build system.
else
    	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
        PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif


PHONY: clean realclean

clean:
	rm -f *.o
	rm -f modules.order Module.symvers
	rm -f *.mod.c

realclean: clean
	rm -f *.ko
