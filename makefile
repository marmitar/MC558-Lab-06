CC = gcc
PROG = t6

OPTIM   = -O3 -march=native -mtune=native -pipe -fivopts
OPTIM  += -fmodulo-sched -flto -fwhole-program -fno-plt
CFLAGS = -std=c99 -Wall -Werror -Wpedantic -Wunused-result $(OPTIM)

VGFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --error-exitcode=10


.PHONY: all clean debug run

all: clean main

debug: OPTIM = -O0 -ggdb3 -DDEBUG
debug: all

main: $(PROG).c
	$(CC) $(CFLAGS) $< -o $@

run: main
	valgrind $(VGFLAGS) ./$<

test: main
	@fish runtest.fish

memcheck: main
	@fish valgrind.fish

clean:
	rm -f main *.valgrind vgcore.*
