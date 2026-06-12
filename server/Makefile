CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -g
PROG=aesdsocket

all: $(PROG)

clean:
	-rm *.o $(PROG)

$(PROG).o:
	$(CC) $(CFLAGS) -c $(PROG).c

$(PROG): $(PROG).o
	$(CC) $(CFLAGS) -o $(PROG) $(PROG).o
