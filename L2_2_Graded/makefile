
editor:
	gcc -Wall -fsanitize=address,undefined -lrt -o editor editor.c

reporter:
	gcc -Wall -fsanitize=address,undefined -lrt -o reporter reporter.c

.PHONY:
	all clean remake

all:
	make editor reporter

clean:
	rm editor reporter;

remake:
	make clean; make all;