#include "socket_lib.h"

#define BACKLOG 3
#define MAX_CON 10
#define WAIT_BUF 128

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
	do_work = 0;
}

struct connection{
    int free;
    int fd;
    int type;
    struct sockaddr addr;
    socklen_t len;
};

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket port\n", name);
}

struct sockaddr_in make_address(char *address, char *port)
{
	int ret;
	struct sockaddr_in addr;
	struct addrinfo *result;
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	if ((ret = getaddrinfo(address, port, &hints, &result))) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}
	addr = *(struct sockaddr_in *)(result->ai_addr);
	freeaddrinfo(result);
	return addr;
}

void client(int cfd, char * buffer){
    // char buffer[6] = "Hello\0";
    // if (bulk_write(cfd, buffer, sizeof(char[5])) < 0 && errno != EPIPE)
    // {
    //     ERR("write");
    // }
    // if (TEMP_FAILURE_RETRY(close(cfd)) < 0)
    // {
    //     ERR("close");
    // }
    int offset = strlen(buffer);
    int n = WAIT_BUF - offset;
    struct sockaddr_in addr;
    socklen_t addrlen;
    recvfrom(cfd, buffer + offset, n, NULL, &addr, &addrlen);
    printf("%d : %d\n", addr.sin_addr, addr.sin_port);
    printf("%s\n", buffer);

}

int find_index(struct connection con[MAX_CON], struct sockaddr addr, socklen_t len , int type){
    int i, empty =-1, pos =-1;
    for (i = 0; i < MAX_CON; i++)
    {
        if (con[i].free == 1)
        {
            empty = i;
        }
        else if (con[i].len == len && 0 == memcmp(&addr, &(con[i].addr), len) )
        {
            pos = i;
            break;
        }
    }
    if (pos == -1 && empty != -1)
    {
        con[empty].free = 0;
        con[empty].addr = addr;
        con[empty].len = len;
        pos = empty;
    }
    return pos;   
}

void doServer(int fdL, int fdT){
    int cfd, fdmax;
	fd_set base_rfds, rfds;
	sigset_t mask, oldmask;

    char wait_buf[WAIT_BUF];

    struct connection con[MAX_CON];

    for (int i = 0; i < MAX_CON; i++)
    {
        con[i].free=1;
    }
    struct sockaddr* addr;
    socklen_t *addrlen;

	FD_ZERO(&base_rfds);
	FD_SET(fdL, &base_rfds);
	FD_SET(fdT, &base_rfds);
	fdmax = (fdT > fdL ? fdT : fdL);
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
    	while (do_work) {
		rfds = base_rfds;
		if (pselect(fdmax + 1, &rfds, NULL, NULL, NULL, &oldmask) > 0) {
			if (FD_ISSET(fdL, &rfds)){
				cfd = add_new_client(fdL, NULL, NULL);
            }
			else{
				cfd = add_new_client(fdT, NULL, NULL);
            }
			if (cfd >= 0)
				client(cfd, wait_buf);
		} else {
			if (EINTR == errno)
				continue;
			ERR("pselect");
		}
	}
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char **argv)
{
	int fdL, fdT;
	int new_flags;
	if (argc != 3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");

	if (sethandler(sigint_handler, SIGINT))
		ERR("Seting SIGINT:");

	fdL = bind_local_socket(argv[1], BACKLOG);
	new_flags = fcntl(fdL, F_GETFL) | O_NONBLOCK;
	fcntl(fdL, F_SETFL, new_flags);

	fdT = bind_tcp_socket(atoi(argv[2]), BACKLOG);
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);

	doServer(fdL, fdT);
	
    if (TEMP_FAILURE_RETRY(close(fdL)) < 0)
		ERR("close");
	if (unlink(argv[1]) < 0)
		ERR("unlink");
	if (TEMP_FAILURE_RETRY(close(fdT)) < 0)
		ERR("close");

	fprintf(stderr, "Server has terminated.\n");
	return EXIT_SUCCESS;
}