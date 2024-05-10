all:
	make prog21a_s prog21_c prog21b_s prog22a prog22b test test2

prog21a_s:
	gcc -Wall -std=gnu99 -o prog21a_s prog21a_s.c

prog21b_s:
	gcc -Wall -std=gnu99 -o prog21b_s prog21b_s.c

prog22a:
	gcc -Wall -std=gnu99 -o prog22a prog22a.c

prog22b:
	gcc -Wall -std=gnu99 -o prog22b prog22b.c

test:
	gcc -Wall -std=gnu99 -o test test.c

test2:
	gcc -Wall -std=gnu99 -o test2 test2.c


prog21_c:
	gcc -Wall -std=gnu99 -o prog21_c prog21_c.c

.PHONY:
	clean all remake

clean:
	rm prog21a_s prog21_c prog21b_s prog22a prog22b test test2

remake:
	make clean; make all;