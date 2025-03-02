.PHONY: all, clean

all:
	gcc -Iinclude -o fibo code/fibo.c code/main.c
	gcc -Iinclude -c code/main.c -o code/main.o
	gcc -Iinclude -c code/fibo.c -o code/fibo.o


clean:
	rm -rf code/*.o
