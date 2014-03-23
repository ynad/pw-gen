## Pw-Gen makefile

target:
	# Main module - Object
	gcc -Wall -I src/ -c src/pw-gen.c
	mkdir -p obj
	mv *.o obj

install: target
	mkdir -p bin
	# Main module - Executable
	gcc obj/pw-gen.o -o bin/pw-gen.bin

clean:
	rm -rf obj

uninstall: clean
	rm -f bin/*
	test -h bin || rmdir bin

