CFLAGS = -Wall -g
LDFLAGS=-lgd -lmysqlclient -g

libhagraph.so: libhagraph.o
	$(CC) -shared libhagraph.c -o libhagraph.so $(CFLAGS) $(LDFLAGS)

clean:
	rm libhagraph.so *.o

install: libhagraph.so
	cp libhagraph.so /usr/lib/
	cp libhagraph.h /usr/include/libhagraph/
