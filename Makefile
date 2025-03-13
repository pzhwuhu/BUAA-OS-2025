.PHONY: clean, run, check

all: check.o
	gcc -I src/include -o out/main src/main.c src/output.c

check:
	gcc -c check.c -o check.o

run: 
	out/main

clean: 
	rm check.o out/main
