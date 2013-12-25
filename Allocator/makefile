# makefile

all: memtest

my_allocator.o : my_allocator.c my_allocator.h
	gcc -std=gnu99 -c -g -lm my_allocator.c

ackerman.o: ackerman.c ackerman.h my_allocator.o
	gcc -std=gnu99 -c -g -lm ackerman.c

memtest: memtest.c ackerman.o my_allocator.o
	gcc -std=gnu99 -lm -o memtest memtest.c my_allocator.o ackerman.o