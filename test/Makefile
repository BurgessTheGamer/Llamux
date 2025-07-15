obj-m += test_dequantization.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

test: all
	sudo insmod test_dequantization.ko
	sleep 1
	sudo dmesg | tail -50
	sudo rmmod test_dequantization