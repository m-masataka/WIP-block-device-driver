KERNEL_DIR = /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(shell pwd)
VERBOSE   := 0

obj-m      := hello.o
oppai-objs := hello_module.o

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(BUILD_DIR) KBUILD_VERBOSE=$(VERBOSE) modules

clean:
	rm -f  *.o *.ko *.mod.c *.symvers *.order .oppai*
	rm -fr .tmp_versions
