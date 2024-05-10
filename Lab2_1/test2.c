#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define CORNERS 3
#define BUF_SIZE 200
#define SAFETY_BOUND 1000000

#if CORNERS <= 1
    #error Invalid number of corners
#endif

#if BUF_SIZE > UCHAR_MAX
    #error Illegal BUF_SIZE value: > UCHAR_MAX
#endif
#if BUF_SIZE > PIPE_BUF-1
    #error Illegal BUF_SIZE value: > PIPE_BUF - 1
#endif

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void close_other_pipes(int pipes[], int rd, int wr){
    for (int i = 0; i < 2* CORNERS; i++)
    {
        if (i != rd && i != wr)
        {
            close(pipes[i]);
            printf("<%d> closed fd %d\n", getpid(), i);
        }
    }
}

int read_from_pipe(int fd, char * buffer, int * value){
    unsigned char c;
    memset(buffer, 0, BUF_SIZE+1);
    int status = read(fd, &c, 1);
    if (status < 0)
    {
        ERR("read");
    }
    if (status == 0)
        return 1;
    if ((status = TEMP_FAILURE_RETRY(read(fd, buffer, c)))<0)
    {
        ERR("read");
    }
    *value = atoi(buffer);
    return 0;
}

int write_to_pipe(int fd, char * buffer, int message){
    memset(buffer, 0, BUF_SIZE + 1);
    sprintf(buffer +1, "%d", message);
    unsigned char c = strlen(buffer+1);
    buffer[0] = c;
    if (TEMP_FAILURE_RETRY(write(fd, buffer, c+1)) < 0)
    {
        if (errno == EPIPE)
        {
            return 1;
        }
        ERR("write");
    }
    return 0;
}

void vary_int(int * value, int lower_bound, int upper_bound){
    *value += lower_bound + rand() % (upper_bound - lower_bound +1);
    if (*value > SAFETY_BOUND || *value < SAFETY_BOUND * (-1))
    {
        printf("process <%d> hit safety bound, message value reset to 1\n", getpid());
        *value = 1;
    }
}

int main (int argc, char ** argv){
    sethandler(SIG_IGN, SIGPIPE);
    int rd, wr, is_primary = 1, message = 1;
    int pipes[2 * CORNERS];
    for (int i = 0; i < CORNERS; i++)
    {
        if(pipe(pipes+i*2)) //[0] reads, [1] writes
            ERR("pipe");
    }
    
    for (int i = 0; i < CORNERS - 1; i++)
    {
        int res = fork();
        if (res == 0) //child
        {
            rd = pipes[2*i];
            wr = pipes[3 + 2*i];
            close_other_pipes(pipes, 2*i, 3+2*i);
            is_primary = 0;
            break;
        }
        else if (res == -1) //fail
        {
            ERR("fork");
        }
    }
    char * buffer = (char *)malloc((BUF_SIZE +1) * sizeof(char));
    if (buffer == NULL)
    {
        ERR("malloc");
    }
    srand(time(NULL)* getpid());
    if (is_primary)
    {
        rd = pipes[2 * CORNERS - 2] ;
        wr = pipes[1];
        close_other_pipes(pipes, 2*CORNERS -2, 1);
        write_to_pipe(wr, buffer, message);
    }
    
    while (1)
    {
        if(read_from_pipe(rd, buffer, &message))
        {
            printf("process <%d> detected a broken pipe on read\n", getpid());
            break;
        }
        if (message == 0)
        {
            printf("process <%d> recieved a 0 message\n\n", getpid());
            break;
        }
        printf("<%d>: %d\n", getpid(), message);
        vary_int(&message, -10, 10);
        if (write_to_pipe(wr, buffer, message))
        {
            printf("process <%d> detected a broken pipe on write\n", getpid());
            break;
        }
        
    }
    
    close(rd);
    close(wr);
    free(buffer);
    if (!is_primary)
    {
        printf("child process <%d> has finished execution normally\n", getpid());
        exit(EXIT_SUCCESS);
    }
    


    while(wait(NULL)>0);
    printf("parent <%d> has waited for all its children and finished execution\n", getpid());
    return EXIT_SUCCESS;
}