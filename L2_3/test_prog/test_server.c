#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define BACKLOG 3
volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
	do_work = 0;
}

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

int make_socket(int domain, int type)
{
	int sock;
	sock = socket(domain, type, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
}

int bind_tcp_socket(uint16_t port)
{
	struct sockaddr_in addr;
	int socketfd, t = 1;
	socketfd = make_socket(PF_INET, SOCK_STREAM);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
		ERR("setsockopt");
	if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR("bind");
	if (listen(socketfd, BACKLOG) < 0)
		ERR("listen");
	return socketfd;
}

int add_new_client(int sfd)
{
	int nfd;
	if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0) {
		if (EAGAIN == errno || EWOULDBLOCK == errno)
			return -1;
		ERR("accept");
	}
	return nfd;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port\n", name);
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (0 == c)
			return len;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if (c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

int16_t calculate(char data[15]){
    int32_t pid = atoi(data);
    
    if (pid == 0)
    {
        printf("Invlid data\n");
        return (int16_t)-1;
    }
    int16_t sum = 0;
    while (pid != 0)
    {
        sum += pid % 10;
        pid /= 10;
    }
    return sum;
}

void communicate(int cfd, int16_t *max)
{
	ssize_t size;
    int16_t sum;
	char data[15];
	if ((size = bulk_read( cfd, (char *)data, sizeof(char[15]) ) ) < 0)
		ERR("read:");

    sum = calculate(data);
    
    *max = *max >= sum ? *max : sum; 

    int16_t ans[1];
    ans[0] = htons(sum);

    if (bulk_write(cfd, (char *)ans, sizeof(int16_t[1])) < 0 && errno != EPIPE)
        ERR("write:");

	if (TEMP_FAILURE_RETRY(close(cfd)) < 0)
		ERR("close");
}

void doServer(int fdT)
{
	int cfd, fdmax;
	fd_set base_rfds, rfds;
	sigset_t mask, oldmask;
    int16_t max=0;
	FD_ZERO(&base_rfds);
	FD_SET(fdT, &base_rfds);
	fdmax = fdT;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	while (do_work) {
		rfds = base_rfds;
		if (pselect(fdmax + 1, &rfds, NULL, NULL, NULL, &oldmask) > 0) {
			cfd = add_new_client(fdT);
			if (cfd >= 0){
				communicate(cfd, &max);
            }
		} else {
			if (EINTR == errno)
				continue;
			ERR("pselect");
		}
	}
    printf("MAX_SUM=%d\n", max);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char **argv)
{
	int fdT;
	int new_flags;
	if (argc != 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	if (sethandler(sigint_handler, SIGINT))
		ERR("Seting SIGINT:");

	fdT = bind_tcp_socket(atoi(argv[1]));
    
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);
	doServer(fdT);
	if (TEMP_FAILURE_RETRY(close(fdT)) < 0)
		ERR("close");
	fprintf(stderr, "Server has terminated.\n");
	return EXIT_SUCCESS;
}