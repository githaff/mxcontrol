# This is test libusb project

CFLAGS = -Wall -pedantic -g
CFLAGS += `pkg-config --cflags hidapi-libusb` -pthread
LDFLAGS += `pkg-config --libs hidapi-libusb` `pkg-config --libs libudev`

mxcontrol : main.c aux.c
	gcc $(CFLAGS) $(LDFLAGS) main.c aux.c -o mxcontrol


clean :
	rm -f *.o
	rm -f mxcontrol

.PHONY : clean
