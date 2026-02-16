CC = gcc
CFLAGS = -Wall -g

all: libmythreads.a

libmythreads.a: libmythreads.o
	rm -f libmythreads.a
	ar -cvrs libmythreads.a libmythreads.o

libmythreads.o: libmythreads.c
	$(CC) $(CFLAGS) -c libmythreads.c -o libmythreads.o 

clean:
	rm -f libmythreads.a *.o

tar:
	tar cvzf project2.tgz README Makefile libmythreads.c
