.PHONY: clean, run, check

all: check.o
	gcc -o out/main src/main.c src/output.c

check:
	gcc -c check.c -o check.o

run: main
	out/main

clean: 
	rm check.o out/main
