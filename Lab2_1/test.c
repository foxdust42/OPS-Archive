#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
//#include <signal.h>
//#include <unistd.h>
//#include <ctype.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#ifndef BUF_SIZE
    #define BUF_SIZE 200 //may not exceed UCHAR_MAX or PIPE_BUF-1, whichever is lowest, enforced below
#endif

#if BUF_SIZE > UCHAR_MAX
    #error Illegal BUF_SIZE value: >UCHAR_MAX
#endif
#if BUF_SIZE > PIPE_BUF-1
    #error Illegal BUF_SIZE value: >PIPE_BUF - 1
#endif

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s <string up to %d chars>\n", name, BUF_SIZE);
	exit(EXIT_FAILURE);
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

int write_to_pipe(int fd, char * message){
    unsigned char c = (unsigned char)strlen(message), buf[BUF_SIZE+1];
    memset(buf, 0, BUF_SIZE+1);
    buf[0] = c; 
    strcat(buf, message);
    if (TEMP_FAILURE_RETRY(write(fd, buf, c+1)) < 0)
    {
        if (errno == EPIPE)
            return 1;
        
        ERR("write");
    }
    return 0;
}

int read_from_pipe(int fd, char * outbuf){
    unsigned char c;
    memset(outbuf, 0, BUF_SIZE);
    int status = read(fd, &c, 1);
    if (status < 0)
    {
        ERR("read");
    }
    if (status == 0)
        return 1;
    if ((status = TEMP_FAILURE_RETRY(read(fd,outbuf, c)))<c)
    {
        ERR("read");
    }
    printf("%s\n", outbuf);
    
    return 0;
}/* code */

void rm_rand_char(char * buf){
    int l = strlen(buf);
    int r = rand() % strlen(buf);
    buf[r] = 0;
    for (unsigned char c = r+1; c < l; c++)
    {
        buf[c-1] = buf[c];
    }
    buf[l-1]=0;
    return;
    
}

int work(int rd, int wr, char * buffer){
    if (read_from_pipe(rd, buffer) == 1)
        return 0;
    if (strlen(buffer)==0)
        return 0;
    rm_rand_char(buffer);

    if (write_to_pipe(wr, buffer) == 1)
        return 0;

    return 1;
}

int main (int argc, char ** argv){
    int p1[2], p2[2]; //[0] reads, [1] writes; p1 -> parent to child, p2 -> child to parent
    int wr, rd;
	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("sethandler");
    if (argc != 2)
    {
        usage(argv[0]);
    }
    if (strlen(argv[1])>BUF_SIZE)
    {
        usage(argv[0]);
    }
    if (pipe(p1) || pipe(p2))
    {
        ERR("pipe");
    }
    char * buffer;
    switch (fork())
    {
    case 0: //child
        close(p1[1]);
        close(p2[0]);
        rd = p1[0];
        wr = p2[1];
        srand(time(NULL)*getpid());
        buffer = (char *)malloc((BUF_SIZE)* sizeof(char));
        
        while (work(rd, wr, buffer));
        close(rd);
        close(wr);
        free(buffer);
        exit(EXIT_SUCCESS);
        break;
    case -1:
        ERR("fork");
    default: //parent
        close(p1[0]);
        close(p2[1]);
        rd = p2[0];
        wr = p1[1];
        srand(time(NULL)*getpid());
        
        write_to_pipe(wr, argv[1]);
        buffer = (char *)malloc((BUF_SIZE)* sizeof(char));
        while (work(rd, wr, buffer));
    }
    close(rd);
    close(wr);
    free(buffer);
    while (wait(NULL)>0);
}