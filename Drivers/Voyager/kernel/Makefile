# Set path to kernel sources include/ directory
KERNEL_INCLUDE=/usr/src/linux/include

KERNEL_VERSION := $(shell grep UTS_RELEASE ${KERNEL_INCLUDE}/linux/version.h |cut -f2 -d'"')

# Where to install the module
MODULE_PATH := /lib/modules/${KERNEL_VERSION}/misc

CC=gcc
LD=ld

CFLAGS := -D__KERNEL__ -DMODULE -O2 \
    -Wall -Wstrict-prototypes -Wno-trigraphs \
    -fomit-frame-pointer -fno-strict-aliasing -fno-common
#CFLAGS += -march=xxx
CFLAGS += -I.
CFLAGS += -I$(KERNEL_INCLUDE)

all: brlvger.o

brlvger.o: brlvger.c linux/brlvger.h
	$(CC) $(CFLAGS) -c brlvger.c

install: brlvger.o
	@if [ ! -d ${MODULE_PATH} ]; then \
	  mkdir -m 755 ${MODULE_PATH} ; \
        fi
	install -m 644 brlvger.o ${MODULE_PATH}
	depmod -a ${KERNEL_VERSION}

vgertest: vgertest.c
	gcc vgertest.c -o vgertest

clean:
	rm -f *.o vgertest

distclean: clean
