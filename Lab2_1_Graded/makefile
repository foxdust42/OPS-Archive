prog:
	gcc -fsanitize=address,undefined -Wall -o prog prog.c

.PHONY:
	clean remake

remake: 
	rm prog; make prog;