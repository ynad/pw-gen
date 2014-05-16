## Pw-Gen makefile

# Compilation variables
CC=gcc
CFLAGS=-Wall -O3 -D_FILE_OFFSET_BITS=64
LDFLAGS=-lm -lrt -lpthread
INCL=-I src/
DEBUG=-g -DDEBUG
SOURCES=src/pw-gen.c src/generator.c src/lib.c
OBJECTS=obj/pw-gen.o obj/generator.o obj/lib.o
EXECUTABLE=bin/pw-gen.bin

target:
	# Main module - Object
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES) $(LDFLAGS)
	mkdir -p obj
	mv *.o obj

install: target
	# Main module - Executable
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

debug:
	# Main module - DEBUG
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES) $(LDFLAGS) $(DEBUG)
	mkdir -p obj
	mv *.o obj
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS) $(DEBUG)

.PHONY: clean

clean:
	rm -rf obj *.o

uninstall: clean
	rm -f bin/*
	test -h bin || rmdir bin

