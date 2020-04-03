.PHONY: clean all

LDFLAGS=
CFLAGS=-Wall -Wextra -Wshadow -Wformat=2 -ansi -O2 -g -Wno-unused-parameter -Wno-comment

all: main clean

main: main.o sculpt.o

sculpt.o: sculpt.c sculpt.h

main.o: main.c

clean:
	@rm -rf main.o sculpt.o
