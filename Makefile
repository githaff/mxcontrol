# This is test libusb project

CFLAGS = -Wall -pedantic -g
CFLAGS += `pkg-config --cflags hidapi-libusb` -pthread
LDFLAGS += `pkg-config --libs hidapi-libusb` `pkg-config --libs libudev`

mxcontrol : main.o aux.o
	gcc $(CFLAGS) $(LDFLAGS) main.o aux.o -o mxcontrol

main.o : main.c
	gcc $(CFLAGS) -c main.c

aux.o : aux.c
	gcc $(CFLAGS) -c aux.c


clean :
	rm -f *.o
	rm -f mxcontrol

.PHONY : clean
