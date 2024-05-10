CC=gcc
CFLAGS=-std=gnu99 -Wall 
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

all: prog17 prog18 prog19 prog20

prog17:
	gcc -std=gnu99 -Wall -fsanitize=address,undefined -lpthread -lm -o prog17 prog17.c
prog18:
	gcc -std=gnu99 -Wall -fsanitize=address,undefined -lpthread -lm -o prog18 prog18.c
prog19:
	gcc -std=gnu99 -Wall -fsanitize=address,undefined -lpthread -lm -o prog19 prog19.c
prog20:
	gcc -std=gnu99 -Wall -fsanitize=address,undefined -lpthread -lm -o prog20 prog20.c
test:
	gcc -std=gnu99 -Wall -fsanitize=address,undefined -lpthread -lm -o test test.c

.PHONY: prog17 prog18 prog19 prog20 clean remake test

all: prog17 prog18 prog19 prog20 test

clean:
	rm prog17 prog18 prog19 prog20

remake:
	make clean; make all;