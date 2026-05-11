CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -g
PROG=writer

all: $(PROG)

clean:
	rm *.o $(PROG)

$(PROG).o:
	$(CC) $(CFLAGS) -c $(PROG).c

$(PROG): $(PROG).o
	$(CC) $(CFLAGS) -o $(PROG) $(PROG).o
