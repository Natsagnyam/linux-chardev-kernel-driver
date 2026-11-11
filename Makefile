obj-m += demodev.o

# Current running kernel build directory
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: module app

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

app: app.c
	gcc app.c -o app

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f app
