bingo:
	gcc -Wall -fsanitize=address,undefined -lrt -o bingo bingo.c

test_server:
	gcc -Wall -fsanitize=address,undefined -lrt -o test_server test_server.c

test_client:
	gcc -Wall -fsanitize=address,undefined -lrt -o test_client test_client.c

pastry_server:
	gcc -Wall -fsanitize=address,undefined -lrt -o pastry_server pastry_server.c

pastry_client:
	gcc -Wall -fsanitize=address,undefined -lrt -o pastry_client pastry_client.c

.PHONY:
	all clean remake

all:
	make bingo test_server test_client pastry_server pastry_client

clean:
	rm bingo test_server test_client pastry_server pastry_client;

remake:
	make clean; make all;