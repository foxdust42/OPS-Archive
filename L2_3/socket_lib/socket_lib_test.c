#include "socket_lib.h"

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

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket port\n", name);
}

void calculate(int32_t data[5])
{
	int32_t op1, op2, result, status = 1;
	op1 = ntohl(data[0]);
	op2 = ntohl(data[1]);
	switch ((char)ntohl(data[3])) {
	case '+':
		result = op1 + op2;
		break;
	case '-':
		result = op1 - op2;
		break;
	case '*':
		result = op1 * op2;
		break;
	case '/':
		if (!op2)
			status = 0;
		else
			result = op1 / op2;
		break;
	default:
		status = 0;
	}
	data[4] = htonl(status);
	data[2] = htonl(result);
}

void communicate(int cfd)
{
	ssize_t size;
	int32_t data[5];
	if ((size = bulk_read(cfd, (char *)data, sizeof(int32_t[5]))) < 0)
		ERR("read:");
	if (size == (int)sizeof(int32_t[5])) {
		calculate(data);
		if (bulk_write(cfd, (char *)data, sizeof(int32_t[5])) < 0 && errno != EPIPE)
			ERR("write:");
	}
	if (TEMP_FAILURE_RETRY(close(cfd)) < 0)
		ERR("close");
}

void doServer(int fdL, int fdT)
{
	int cfd, fdmax;
	fd_set base_rfds, rfds;
	sigset_t mask, oldmask;
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
			if (FD_ISSET(fdL, &rfds))
				cfd = add_new_client(fdL);
			else
				cfd = add_new_client(fdT);
			if (cfd >= 0)
				communicate(cfd);
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