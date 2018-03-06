# This is test libusb project

CFLAGS = -Wall -pedantic
CFLAGS += `pkg-config --cflags hidapi-libusb` -g
LDFLAGS += `pkg-config --libs hidapi-libusb`

usb : main.o usb.o aux.o
	gcc $(LDFLAGS) main.o usb.o aux.o -o usb

xusb : xusb.c
	gcc -Wall -pedantic `pkg-config --cflags --libs libusb-1.0` -o xusb xusb.c

main.o : main.c
	gcc $(CFLAGS) -c main.c

usb.o : usb.c
	gcc $(CFLAGS) -c usb.c

aux.o : aux.c
	gcc $(CFLAGS) -c aux.c


clean :
	rm -f *.o
	rm -f usb xusb

.PHONY : clean
