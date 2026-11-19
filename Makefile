CC = gcc
CFLAGS ?= -std=c2x -Wall -Wextra

SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h)

ifj26: $(SOURCES) $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(SOURCES) -o $@
