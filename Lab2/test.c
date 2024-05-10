#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal=0;

void sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void sig_handler(int sig)
{
	last_signal = sig;
}

void sigchld_handler(int sig)
{
	pid_t pid;
	for (;;) {
		pid = waitpid(0, NULL, WNOHANG);
		if (pid == 0)
			return;
		if (pid <= 0) {
			if (errno == ECHILD)
				return;
			ERR("waitpid");
		}
	}
}

void child_work(int i)
{
	srand(time(NULL) * getpid());
	int s = 100 + rand() % (200 - 100 + 1);
    struct timespec ts = { 0, s };

	//printf("Child with pid %d says: %d\n", getpid(), s);
    for (;;)
    {
        nanosleep(&ts, NULL);
        if (kill(getppid(),SIGUSR1) < 0)
        {
            ERR("kill");
        }
        //printf("*");
    }
    
}

void parent_work(sigset_t oldmask)
{

	int count = 0;
	while (1) {
		last_signal = 0;
		while (last_signal != SIGUSR1)
			sigsuspend(&oldmask);
		count++;
		printf("[PARENT] received %d SIGUSR1\n", count);
		if (count >= 100)
		{
			return;
		}
		
	}
}


void create_children(int n)
{
	pid_t s;
	for (n--; n >= 0; n--) {
		if ((s = fork()) < 0)
			ERR("Fork:");
		if (!s) {
			child_work(n);
			exit(EXIT_SUCCESS);
		}
	}
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s 0<n\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int n;
	if (argc != 2)
		usage(argv[0]);
	n = atoi(argv[1]);
	if (n <= 0)
		usage(argv[0]);
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(sig_handler, SIGUSR1);

    sigset_t mask, oldmask;
	sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
	//sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

	create_children(n);
    parent_work(oldmask);
	sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	if (kill(0,SIGUSR2)<0)
	{
		ERR("kill");
	}
	
    while (wait(NULL) > 0);
	return EXIT_SUCCESS;
}
