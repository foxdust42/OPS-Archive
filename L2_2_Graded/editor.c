#define _GNU_SOURCE

#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t sigint_rec = 0;
volatile sig_atomic_t sigusr1_rec = 0;

void mq_close_unlink(mqd_t qid, char * name){
    mq_close(qid);
    if (mq_unlink(name)){
        if (errno != ENOENT)
        {
            ERR("mq_unlink");
        }
        
    }
}

void sethandler( void (*f)(int), int sigNo) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sigint_handler(int sig){
    sigint_rec = 1;
}

void sigusr1_handler(int sig){
    sigusr1_rec = (sigusr1_rec + 1)%2;
    if (sigusr1_rec == 1)
    {
        printf("editor sleeps\n");
    }
    else
    {
        printf("editor reads\n");
    }
    
    
}

void check_input(int argc, char ** argv){
    if (argc != 2)
    {
        printf("Improper input\n");
        exit(EXIT_FAILURE);
    }
}

void open_create_queue(mqd_t * qid, char * name){

    struct mq_attr attr;
    attr.mq_maxmsg = 3;
    attr.mq_msgsize = 12;

    if ((*qid = TEMP_FAILURE_RETRY(mq_open(name, O_RDWR | O_CREAT | O_NONBLOCK, 0600, &attr))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
}

int recieve_message(mqd_t qid, char * message, int len){

    if (TEMP_FAILURE_RETRY(mq_receive(qid, message, len, NULL)) < 0 )
    {   
        if (errno == EAGAIN)
        {
            return -1;
        }
        
        ERR("mq_recieve");
    }
    return 0;
}

void print_buffer(char * message, int len){
    for (int i = 0; i < len; i++)
    {
        printf("%c", message[i]);
    }
    printf("\n");
}

int main(int argc, char ** argv){
    mqd_t qid = -1;
    
    int offset = 0;
    char articles[12 * 10];
    int res=0;
    
    check_input(argc, argv);
    open_create_queue( &qid, argv[1]);

    printf("Server stands on: %d\n", getpid());

    sethandler(sigint_handler, SIGINT);
    sethandler(sigusr1_handler, SIGUSR1);
    while (!sigint_rec)
    {
        if (sigusr1_rec == 1)
        {
            continue;
        }
        
        res = recieve_message(qid, articles + (offset * 12), 12);
        
        if (offset >= 9 && res == 0)
        {
            offset = -1;
            print_buffer(articles, 12 * 10);
        }
        if (res == 0)
        {
            offset++;
        }
        
    }
    
    printf("\nterminating...\n");

    mq_close_unlink(qid, argv[1]);
    return EXIT_SUCCESS;
}