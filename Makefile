# This is test libusb project

CFLAGS = -Wall -pedantic
CFLAGS += `pkg-config --cflags hidapi-hidraw`
LDFLAGS += `pkg-config --libs hidapi-hidraw`


usb : main.o usb.o aux.o
	gcc $(LDFLAGS) main.o usb.o aux.o -o usb

main.o : main.c
	gcc $(CFLAGS) -c main.c

usb.o : usb.c
	gcc $(CFLAGS) -c usb.c

aux.o : aux.c
	gcc $(CFLAGS) -c aux.c


clean :
	rm -f *.o
	rm usb

.PHONY : clean
