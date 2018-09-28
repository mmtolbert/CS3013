LIB=-lpthread
CC=gcc
CCPP=g++

all: life addem

life: life.c
	$(CC) life.c -o life $(LIB)
addem: addem.c
	$(CC) addem.c -o addem $(LIB)
clean:
	rm -f *.o life addem
