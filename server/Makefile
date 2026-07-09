CC ?= $(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -std=gnu11 -D_GNU_SOURCE -I/usr/include -g
LDFLAGS=-lpthread
PROG=aesdsocket

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)


all: $(PROG)

clean:
	-rm *.o $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
