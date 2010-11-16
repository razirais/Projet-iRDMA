BUILDROOT=/tp-fmr/linux/buildroot-2010.08
CROSS_COMPILE=$(BUILDROOT)/output/staging/usr/bin/powerpc-linux-
PATH+=:$(BUILDROOT)/output/staging/usr/bin/:

CROSS=powerpc-linux-uclibc-

export CROSS_COMPILE
export BUILDROOT
export PATH

CC=$(CROSS_COMPILE)gcc

LIN_DIST=/tp-fmr/linux/linux-2.6-xlnx


VPATH=appli

all : module install


module :
	echo "$(CROSS_COMPILE)"
	CROSS_COMPILE=$(CROSS_COMPILE) \
	make -C $(LIN_DIST) \
	ARCH=powerpc M=`pwd`/driver modules


clean : 
	rm $(APPLIS)
	rm driver/*.o driver/*.ko
