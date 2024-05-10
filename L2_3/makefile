calculator_server:
	gcc -Wall -std=gnu99 -o calculator_server calculator_server.c

calculator_client_local:
	gcc -Wall -std=gnu99 -o calculator_client_local calculator_client_local.c

calculator_client_tcp:
	gcc -Wall -std=gnu99 -o calculator_client_tcp calculator_client_tcp.c

file_server:
	gcc -Wall -std=gnu99 -o file_server file_server.c

file_client_udp:
	gcc -Wall -std=gnu99 -o file_client_udp file_client_udp.c

test_server:
	gcc -Wall -std=gnu99 -o test_prog/test_server test_prog/test_server.c

test_client:
	gcc -Wall -std=gnu99 -o test_prog/test_client test_prog/test_client.c

.PHONY:
	all clean remake

all:
	make calculator_server calculator_client_local calculator_client_tcp file_client_udp file_server test_client test_server

clean:
	rm calculator_client_local calculator_client_tcp calculator_server file_client_udp file_server test_client test_server

remake:
	make clean; make all;