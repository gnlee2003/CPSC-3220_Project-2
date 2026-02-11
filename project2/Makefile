
CC=clang
CFLAGS=-Wall -g
BINS=contextfun funcptr makecontext_example


all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -rf $(BINS) *.o *.dSYM
