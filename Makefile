obj-m += demodev.o

all: module app stress_app

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

app: app.c
	gcc app.c -o app

stress_app: stress_app.c
	gcc -pthread stress_app.c -o stress_app

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f app stress_app