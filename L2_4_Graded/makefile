all:
	make server

server:
	gcc -g -Wall -fsanitize=address,undefined -lrt -lpthread -o server.o server.c

.PHONY:
	all clean remake

clean:
	rm *.o

remake:
	make clean; make remake;
