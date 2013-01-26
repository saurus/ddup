CC=gcc
CFLAGS=-std=c99 -g -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -Wall -W
LDLIBS=-lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  -lssl -lcrypto
# -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0    -lnsl -lgnutls -lgcrypt -lz -lgpg-error   -lpthread   -lgnutls -lgcrypt -lgpg-error -lz -ldb-4.8 -pthread -lsasl2    -lgcrypt -lgpg-error  -lgnutls -lgcrypt -lgpg-error -lz -ldb-4.8 -pthread -lsasl2

ddup: main.o
	$(CC) -o ddup main.o $(LDFLAGS) $(LDLIBS)
