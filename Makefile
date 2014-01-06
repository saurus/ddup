CC=gcc
CFLAGS=-std=c99 -g -Wall -W $(shell pkg-config --cflags glib-2.0) $(shell pkg-config --libs libssl)
LDLIBS=-lssl -lcrypto $(shell pkg-config --libs glib-2.0) $(shell pkg-config --libs libssl)

ddup: main.o format.o
	$(CC) -o ddup main.o format.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o *.a ddup
