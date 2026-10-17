CC = gcc
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu2x -fPIC -O2
LDFLAGS = -shared

.PHONY: all clean

all: librstack.so

librstack.so: rstack.o
	$(CC) $(LDFLAGS) -o $@ $^

rstack.o: rstack.c rstack.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o librstack.so