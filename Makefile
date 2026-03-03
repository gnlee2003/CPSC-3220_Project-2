CC = gcc
CFLAGS = -Wall -g

all: libmythreads.a #tests

libmythreads.a: libmythreads.o
	rm -f libmythreads.a
	ar -cvrs libmythreads.a libmythreads.o

libmythreads.o: libmythreads.c
	$(CC) $(CFLAGS) -c libmythreads.c -o libmythreads.o 

tests: cooptest locktest preempttest

cooptest: cooperative_test.c libmythreads.o
	$(CC) $(CFLAGS) -o $@ $^ 

locktest: lock_test.c libmythreads.o
	$(CC) $(CFLAGS) -o $@ $^

preempttest: preemptive_test.c libmythreads.o
	$(CC) $(CFLAGS) -o $@ $^  

clean:
	rm -f libmythreads.a *.o cooptest locktest preempttest core *.core

tar:
	tar cvzf project2.tgz README Makefile libmythreads.c
