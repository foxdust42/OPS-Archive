#define _GNU_SOURCE
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

int make_socket(void)
{
	int sock;
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
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

int connect_socket(char *name, char *port)
{
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket();
	addr = make_address(name, port);
	if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		if (errno != EINTR)
			ERR("connect");
		else {
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
				ERR("select");
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("getsockopt");
			if (0 != status)
				ERR("connect");
		}
	}
	return socketfd;
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

void prepare_request(char data[15])
{
    memset(data, 0, sizeof(char[15]));
    sprintf(data, "%d", getpid());
    printf("PID=%s\n", data);
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port\n", name);
}

int main(int argc, char **argv)
{
	int fd;
	char data[15];
    int16_t ans[1];
	if (argc != 3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
    
	fd = connect_socket(argv[1], argv[2]);

	prepare_request(data);

	if (bulk_write(fd, (char *)data, sizeof(char[15])) < 0)
    {
        ERR("write:");
    }
    
	if (bulk_read(fd, (char *)ans, sizeof(int16_t[1])) < (int)sizeof(int16_t[1]))
    {
	    ERR("read:");
    }
    
	printf("SUM=%d\n", ntohs(ans[0]));

    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
    {
		ERR("close");
    }
	return EXIT_SUCCESS;
}