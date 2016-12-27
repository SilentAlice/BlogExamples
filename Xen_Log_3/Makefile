#obj-m += hypercall_ko_ubuntu.o
obj-m += hypercall_ko_debian.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
