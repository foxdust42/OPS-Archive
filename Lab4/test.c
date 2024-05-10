#define _GNU_SOURCE
#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

struct thread_args{
    int fd;
    pthread_t parent_id;
    char* buf;
    int blocksize;
    int offset;
};

void usage(char *);
off_t getfilelength(int fd);
void aiocb_init(struct aiocb *aiocbs, int fd, int blocksize, char * buffer, struct thread_args *ta);
void aio_notify_function(union sigval sv);
void suspend(struct aiocb *aiocbs);
void alter_block(char * buffer, int blocksize);
void sethandler(void (*f)(int), int sigNo);
void sigio_handler(int i);
void next_block(struct aiocb * aiocbs, int blocksize, struct thread_args *ta);

volatile sig_atomic_t sigio_rec;

int main(int argc, char ** argv){
    char * filename, *buffer; 
    int n, fd, blocksize, remsize, i;
    sigset_t newmask, oldmask;
    if (argc != 3){usage(argv[0]);}
    filename = argv[1];
    if ( (n = atoi(argv[2])) <= 0){usage(argv[0]);}
    if ( (fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1 ){ERR("open");}
    
    blocksize = (getfilelength(fd) ) / n;
    remsize = (getfilelength(fd) ) % n; 
    //if (getfilelength(fd)%n !=0){Rem=1;}
    if( (buffer = (char*)malloc(sizeof(char)*blocksize)) == NULL ){ERR("malloc");}

    fprintf(stdout, "Total size: %ld\n", getfilelength(fd));
    fprintf(stdout, "Blocks 0 - %d : %d\n", n, blocksize);
    fprintf(stdout, "Remiander block (%d) : %d\n", n+1, remsize);
    fprintf(stdout, "Last character omitted from operation\n");

    struct thread_args *ta = (struct thread_args *)malloc(sizeof(struct thread_args));
    if(ta == NULL){ERR("malloc");}
    
    struct aiocb io_control;
    aiocb_init(&io_control, fd, blocksize, buffer, ta);
    sigio_rec=0;
    sethandler(sigio_handler, SIGIO);
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGIO);

    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    for ( i = 0; i < n; i++)
    {
        fprintf(stdout, "Block %d\n", i);
        aio_read(&io_control);
        while ( sigio_rec == 0)
        {
            sigsuspend(&oldmask);
        }
        suspend(&io_control);
        sigio_rec=0;
        next_block(&io_control, blocksize, ta);
    }
    
        fprintf(stdout, "Remainder\n");
        io_control.aio_nbytes=remsize;
        ta->blocksize=remsize;
        aio_read(&io_control);
        while ( sigio_rec == 0)
        {
            sigsuspend(&oldmask);
        }
        suspend(&io_control);

    free(buffer);
    free(ta);
    close(fd);
    return EXIT_SUCCESS;
}

void usage(char *progname)
{
	fprintf(stderr, "%s filename n\n", progname);
	fprintf(stderr, "filename - path to the file to work on\n");
    fprintf(stderr, "n - number of blocks\n");
	exit(EXIT_FAILURE);
}

off_t getfilelength(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf) == -1)
		ERR("fstat");
	return buf.st_size;
}

void aiocb_init(struct aiocb *aiocbs, int fd, int blocksize, char *buffer, struct thread_args *ta){

    ta->blocksize=blocksize;
    ta->buf = buffer;
    ta->parent_id = pthread_self();
    ta->offset=0;
    ta->fd=fd;

    memset(aiocbs, 0, sizeof(struct aiocb));
    aiocbs->aio_fildes = fd;
    aiocbs->aio_offset = 0;
    aiocbs->aio_nbytes = blocksize;
    aiocbs->aio_buf = (void *)buffer;
    aiocbs->aio_sigevent.sigev_notify = SIGEV_THREAD;
    aiocbs->aio_sigevent.sigev_notify_function = aio_notify_function;
    aiocbs->aio_sigevent.sigev_value.sival_int=0;
    aiocbs->aio_sigevent.sigev_value.sival_ptr=ta;
}

void aio_notify_function(union sigval sv){
    int i = 0, w;
    struct thread_args * ta = (struct thread_args *)sv.sival_ptr;
    fprintf(stdout, "Inital state:\n");
    while (i != ta->blocksize)
    {
        fprintf(stdout, "%c", ta->buf[i]);
        i++;
    }
    alter_block(ta->buf, ta->blocksize);
    i=0;
    fprintf(stdout, "\nAltered block:\n");
    while (i != ta->blocksize)
    {
        fprintf(stdout, "%c", ta->buf[i]);
        i++;
    }
    fprintf(stdout,"\n");
    fflush(stdout);
    lseek(ta->fd, ta->offset, SEEK_SET);
    w = write(ta->fd, ta->buf, ta->blocksize);
    if(w==-1){ERR("write");}
    printf("%d\n",w);
    pthread_kill(ta->parent_id, SIGIO);
    return;
}

void suspend(struct aiocb *aiocbs)
{
	struct aiocb *aiolist[1];
	aiolist[0] = aiocbs;
	while (aio_suspend((const struct aiocb *const *)aiolist, 1, NULL) == -1) {
		if (errno == EINTR)
			continue;
		ERR("aio_suspend");
	}
	if (aio_error(aiocbs) != 0)
		ERR("aio_error");
	if (aio_return(aiocbs) == -1)
		ERR("aio_return");
}

void alter_block(char * buffer, int blocksize){
    int i = 0; char c;
    while (i != blocksize){
        c = buffer[i]; 
        if ( !((c >= 'A' && c <= 'Z') || (c>='a' && c<='z') || c=='\n') )
        {
            buffer[i] = ' ';
        }
        i++;
    }
}
void sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void sigio_handler(int i){
    sigio_rec = 1;
    return;
}

void next_block(struct aiocb * aiocbs, int blocksize, struct thread_args *ta){
    aiocbs->aio_offset+=blocksize;
    ta->offset+=blocksize;
}