CC=gcc
CFLAGS=

all: my_shell

my_shell: my_shell.c
	gcc -std=gnu99 -Wall -o my_shell my_shell.c

clean:
	rm -f my_shell *.o
