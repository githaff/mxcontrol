# This is test libusb project

CFLAGS = -Wall -pedantic
CFLAGS += `pkg-config --cflags libusb-1.0`
LDFLAGS += `pkg-config --libs libusb-1.0`

xusb : xusb.c
	gcc $(CFLAGS) $(LDFLAGS) -o xusb xusb.c

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
