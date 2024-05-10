POSIX_sem_server:
	gcc -Wall -lpthread -o POSIX_sem_server.o POSIX_sem_server.c

POSIX_sem_client:
	gcc -Wall -lpthread -o POSIX_sem_client.o POSIX_sem_client.c

POSIX_cond_server:
	gcc -Wall -lpthread -o POSIX_cond_server.o POSIX_cond_server.c

POSIX_cond_client:
	gcc -Wall -lpthread -o POSIX_cond_client.o POSIX_cond_client.c

.PHONY:
	all clean remake

all:
	make POSIX_sem_client POSIX_sem_server POSIX_cond_client POSIX_cond_server

clean:
	rm *.o

remake:
	make clean; make all;