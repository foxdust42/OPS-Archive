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

volatile sig_atomic_t sigint_rec = 0, sigusr1_rec = 0;

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
    sigusr1_rec+=2;
}

void initial_request(mqd_t request_mq){
    char spec_cookie[2] = {0, 0};
    while (1)
    {
        if (TEMP_FAILURE_RETRY(mq_send(request_mq, spec_cookie, 2, 1)) < 0) //Initial cookie
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            
            ERR("mq_send");
        }
        break;
    }
}

void get_rand_cookie(char cookie[2]){
    cookie[0] = rand()%11;
    cookie[1] = rand()&11;
}

void sigusr1_add(mqd_t cookie_mq){
    char spec_cookie[2] = {1,2};
    while (sigusr1_rec--)
    {
        if (TEMP_FAILURE_RETRY(mq_send(cookie_mq, spec_cookie, 2, 1)) < 0) //Initial cookie
        {
            if (errno == EAGAIN)
            {
                sigusr1_rec = 0;
                break;
            }
            
            ERR("mq_send");
        }
        printf("Added SIGUSR1 cookie\n");
    }
}

void random_add(mqd_t cookie_mq, char cookie[2]){
    get_rand_cookie(cookie);

    if (TEMP_FAILURE_RETRY(mq_send(cookie_mq, cookie, 2, 1)) < 0)
    {
        if (errno != EAGAIN)
        {
            ERR("mq_send");
        }
    }
}

int main(int argc, char ** argv){
    char request_queue_name[15] = "/request_queue"; 
    char cookie_queue_name[14] = "/cookie_queue";

    mqd_t request_mq, cookie_mq;

    struct mq_attr attr;
    
    //char spec_cookie[2] = {1, 2};
    char cookie[2];
    srand(getpid() * time(NULL));

    sethandler(sigint_handler, SIGINT);
    sethandler(sigusr1_handler, SIGUSR1);

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 2;

    if ((request_mq = TEMP_FAILURE_RETRY(mq_open(request_queue_name, O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
    if ((cookie_mq = TEMP_FAILURE_RETRY(mq_open(cookie_queue_name, O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }

    printf("server active on pid: %d\n", getpid());

    initial_request(request_mq);

    while (!sigint_rec)
    {
        if (sigusr1_rec > 0)
        {
            sigusr1_add(cookie_mq);
        }
        
        while (1)
        {
            if (TEMP_FAILURE_RETRY(mq_receive(request_mq, cookie, 2, NULL)) < 0 )
            {
                if (errno == EAGAIN)
                {
                    break;
                }
                
                ERR("mq_recieve");
            }
            random_add(cookie_mq, cookie);
        }
    }
    
    printf("\nGoodbye\n");

    mq_close(request_mq);
    mq_close(cookie_mq);

    if (mq_unlink(request_queue_name)){
        ERR("mq_unlink");
    }
    if (mq_unlink(cookie_queue_name)){
        ERR("mq_unlink");
    }
    return EXIT_SUCCESS;
}