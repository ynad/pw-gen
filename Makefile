## Pw-Gen makefile

target:
	# Main module - Object
	gcc -Wall -O3 -I src/ -c src/pw-gen.c src/generator.c src/lib.c -D_FILE_OFFSET_BITS=64 -lm -lrt
	mkdir -p obj
	mv *.o obj

install: target
	# Main module - Executable
	mkdir -p bin
	gcc -Wall -O3 obj/pw-gen.o obj/generator.o obj/lib.o -o bin/pw-gen.bin -D_FILE_OFFSET_BITS=64 -lm -lrt

debug:
	# Main module - DEBUG
	gcc -g -Wall -O3 -I src/ -c src/pw-gen.c src/generator.c src/lib.c -D_FILE_OFFSET_BITS=64 -lm -lrt -DDEBUG
	mkdir -p obj
	mv *.o obj
	mkdir -p bin
	gcc -g -Wall -O3 obj/pw-gen.o obj/generator.o obj/lib.o -o bin/pw-gen.bin -D_FILE_OFFSET_BITS=64 -lm -lrt -DDEBUG

clean:
	rm -rf obj

uninstall: clean
	rm -f bin/*
	test -h bin || rmdir bin

