server:
	gcc -g -Wall -fsanitize=address,undefined -std=gnu99 -o server server.c

.PHONY:
	all clean remake

all:
	make server

clean:
	rm server

remake:
	make clean; make all;