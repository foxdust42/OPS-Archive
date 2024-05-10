#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_SIZE 200

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

void null_handler(int sig){
    return;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s <path to fifo> <1 to 10 elements>\n", name);
	exit(EXIT_FAILURE);
}

// void close_other_pipes(int pipes[], int rd, int wr){
//     for (int i = 0; i < 2/* times size*/; i++)
//     {
//         if (i != rd && i != wr)
//         {
//             close(pipes[i]);
//             printf("<%d> closed fd %d\n", getpid(), i);
//         }
//     }
// }

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

int write_to_pipe(int fd, char * buffer, int message, int ind){
    memset(buffer, 0, BUF_SIZE + 1);
    sprintf(buffer +2, "%d", message);
    unsigned char c = strlen(buffer+2);
    buffer[0] = c;
    buffer[1] = ind;
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

int parent_send_start(int no_children, int fd, int ind){
    unsigned char mess[2];
    mess[0]='I';
    mess[1] = ind;
    for (int i = 0; i < no_children; i++)
    {
        if (TEMP_FAILURE_RETRY(write(fd, mess, 2)) < 0)
        {
            if (errno == EPIPE)
            {
                return 1;
            }
            ERR("write");
        }
    }
    return 0;
}

int parent_read_from_child(int fd, char * buffer, int* arr){
    unsigned char c;
    printf("Test4\n");
    memset(buffer, 0, BUF_SIZE);
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
    int ind = buffer[0];
    int val = atoi(buffer+1);

    printf("parent recieved %d from child %d\n", val, ind);

    arr[ind] = val;
    return 0;
}

int child_read_fifo(int fd, int * ind){
    unsigned char mess[2];
    printf("Test3\n");
    int status = read(fd, &mess, 2);
    if (status < 0)
    {
        ERR("read");
    }
    printf("<%d>:%d", getpid(),  mess[1]);
    *ind = mess[1];
    return 0;
}

int child_work(char * path, int ind, int elem){

    int fifo_rd;
    if ((fifo_rd = open(path, O_RDONLY)) < 0)
        ERR("open");
    int fifo_wr;
    	if ((fifo_wr = open(path, O_WRONLY)) < 0)
		ERR("open");

    char * buf = (char *)malloc((BUF_SIZE +1)*sizeof(char));
    if (!buf)
    {
        ERR("malloc");
    }
    int index;
    child_read_fifo(fifo_rd, &index);

    printf("child <%d> recieved the index %d\n", getpid(), index);

    write_to_pipe(fifo_wr, buf, elem, ind);
    
    close(fifo_rd);
    close(fifo_wr);

    free(buf);
    return 0;
}

int main(int argc, char ** argv){
    if (argc <= 2 || argc > 12)
    {
        usage(argv[0]);
    }
    sethandler(SIG_IGN, SIGPIPE);
    sethandler(null_handler, SIGUSR1);
        
	if (mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
		if (errno != EEXIST)
			ERR("create fifo");



    int no_children = argc -2;    
    int ind, elem;
    for (ind = 0; ind < no_children; ind++)
    {
        int result = fork();
        if (result < 0)
        {
            ERR("fork");
        }
        else if (result == 0) //child
        {
            elem = atoi(argv[2+ind]);
            child_work(argv[1], ind, elem);
            exit(EXIT_SUCCESS);
            break;
        }
        else{ //parent
            
        }
    }
    char * buf = (char *)malloc((BUF_SIZE +1)*sizeof(char));
    if (!buf)
    {
        ERR("malloc");
    }
    int * arr = (int*)malloc((no_children)*sizeof(int));
    
    int fifo_wr;
    	if ((fifo_wr = open(argv[1], O_WRONLY)) < 0)
		ERR("open");
    int fifo_rd;
    if ((fifo_rd = open(argv[1], O_RDWR)) < 0)
		ERR("open");

    parent_send_start(no_children, fifo_wr, 11);

    kill(0, SIGUSR1);
    for (int i = 0; i < no_children; i++)
    {
        parent_read_from_child(fifo_rd, buf, arr);
    }
    

    free(arr);
    free(buf);
    while(wait(NULL)>0);
    close(fifo_rd);
    close(fifo_wr);
    return EXIT_SUCCESS;
}