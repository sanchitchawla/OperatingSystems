CC=gcc
CFLAGS=-Wall -w -pedantic -g


SRC = icsh.c
BIN = icsh

all: exec


exec: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	$(RM) $(BIN)

.PHONY: all clean debug