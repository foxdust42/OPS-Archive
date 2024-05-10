CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog13a prog13b prog14 prog15 prog16a prog16b test

prog13a:
	gcc -Wall -fsanitize=address,undefined -o prog13a prog13a.c

prog13b:
	gcc -Wall -fsanitize=address,undefined -o prog13b prog13b.c

prog14:
	gcc -Wall -fsanitize=address,undefined -o prog14 prog14.c

prog15:
	gcc -Wall -fsanitize=address,undefined -o prog15 prog15.c

prog16a:
	gcc -Wall -fsanitize=address,undefined -o prog16a prog16a.c

prog16b:
	gcc -Wall -fsanitize=address,undefined -o prog16b prog16b.c

test:
	gcc -Wall -fsanitize=address,undefined -o test test.c

.PHONY: clean all remake

clean:
	rm prog13a prog13b prog14 prog15 prog16a prog16b test
remake:
	make clean; make all