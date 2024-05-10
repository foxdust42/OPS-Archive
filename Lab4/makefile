archunix5a:
	gcc -Wall -fsanitize=address,undefined -lrt -o archunix5a archunix5a.c
test:
	gcc -Wall -fsanitize=address,undefined -lrt -lpthread -o test test.c
.PHONY:
	clean remake all
clean:
	rm archunix5a test;
all:
	make archunix5a; make test;
remake:
	make clean; make all;