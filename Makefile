.PHONY: run clean stop client all

obj-m = client.o
client-objs = sources/client/client.o
EXTRA_CFLAGS = -Wall -g -Werror -std=gnu17

KVER=$(shell basename /usr/src/linux-headers-*)
LDFLAGS= -L/usr/lib -llibssh
all: client server
clean:
	make -C /usr/src/$(KVER) M=$(PWD) clean
	rm server

client:
	make -C /usr/src/$(KVER) M=$(PWD) modules


runc: stopc client
	insmod client.ko
	mount -t lab4 192.168.65.3:50000 /mnt/nfs1/
stopc:
	umount /mnt/nfs1 || echo
	rmmod client || echo

server: sources/server/main.c
	gcc $(EXTRA_CFLAGS) -o server sources/server/main.c
runs: stops server
	./server ../test/ 50000
stops:
	pkill "./server" || echo

test: runc
	#------- test start -------
	mkdir /mnt/nfs1/subdir
	rm /mnt/nfs1/test.txt || echo
	touch /mnt/nfs1/test.txt
	echo "Hello World!" > /mnt/nfs1/test.txt
	cat /mnt/nfs1/test.txt
	tree /mnt/nfs1
	rm /mnt/nfs1/test.txt
	rmdir /mnt/nfs1/subdir
	ls -la /mnt/nfs1
