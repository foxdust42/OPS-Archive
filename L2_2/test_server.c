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

struct message
{
    pid_t client_pid;
    int num1;
    int num2;
};

volatile sig_atomic_t end = 0;

void sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;
        if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sigint_handler(int sig){
    end = -1;
}

int main(int argc, char **argv){

    sethandler(sigint_handler, SIGINT);

    pid_t pid = getpid();



    char name_s[15+2]; 
    char name_d[15+2]; 
    char name_m[15+2]; 
    char name_c[15];

    sprintf(name_s, "/%d_s", pid);
    sprintf(name_d, "/%d_d", pid);
    sprintf(name_m, "/%d_m", pid);

    mqd_t qs, qd, qm, qc = -1;

    struct mq_attr attr;

    attr.mq_maxmsg = 5;
    attr.mq_msgsize = sizeof(struct message);

    if (TEMP_FAILURE_RETRY((qs = mq_open(name_s, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
    if (TEMP_FAILURE_RETRY((qd = mq_open(name_d, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
    if (TEMP_FAILURE_RETRY((qm = mq_open(name_m, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }

    //printf("own open\n");

    printf("%s\n", name_s);
    printf("%s\n", name_d);
    printf("%s\n", name_m);




    struct message msg;
    
    while (end != -1)
    {
        if (end == 0)
        {        
            if (TEMP_FAILURE_RETRY(mq_receive(qs, (char*)&msg, sizeof(struct message), NULL)-end) < 0 && end == 0)
            {
                if (end != -1)
                {
                    ERR("mq_recieve");
                }
            }
        }
        //printf("message_recieved\n");
        sprintf(name_c, "/%d", msg.client_pid);

        //printf("%d, %d, %d\n", msg.client_pid, msg.num1, msg.num2);

        //printf("%d - %s\n", msg.client_pid, name_c);
        if (msg.client_pid == -1)
        {
            mq_close(qc);
            qc = -1;
            continue;
        }

        if (qc == -1 && end == 0)
        {        
            if (TEMP_FAILURE_RETRY((qc = mq_open(name_c, O_WRONLY))) == (mqd_t)-1)
            {
                ERR("mq_open");
            }
        }    

        

        int ans = msg.num1 + msg.num2;

        if (end == 0)
        {
            if (TEMP_FAILURE_RETRY(mq_send(qc, (const char *)&ans, sizeof(int), 1)) < 0 && end == 0)
            {
                if (end != -1)
                {
                    ERR("mq_send");
                }
            }
        }


        if (end == 0)
        {    
            if (TEMP_FAILURE_RETRY(mq_receive(qd, (char*)&msg, sizeof(struct message), NULL)-end) < 0 && end == 0)
            {
                if (end != -1)
                {
                    ERR("mq_recieve");
                }
            }
        }
        //printf("message_recieved\n");

        if (msg.client_pid == -1)
        {
            mq_close(qc);
            qc = -1;
            continue;
        }

        ans = msg.num1 / msg.num2;

        if (end == 0)
        {
            if (TEMP_FAILURE_RETRY(mq_send(qc, (const char *)&ans, sizeof(int), 1)) < 0 && end == 0)
            {
                if (end != -1)
                {
                    ERR("mq_send");
                }
            }
        }

        if (end == 0)
        {
            if (TEMP_FAILURE_RETRY(mq_receive(qm, (char*)&msg, sizeof(struct message), NULL)-end) < 0 && end == 0)
            {
                if (end != -1)
                {
                    ERR("mq_recieve");
                }
            }
        }
        //printf("message_recieved\n");

        if (msg.client_pid == -1)
        {
            mq_close(qc);
            qc = -1;
            continue;
        }

        ans = msg.num1 % msg.num2;



        if (end == 0)
        {
            if (TEMP_FAILURE_RETRY(mq_send(qc, (const char *)&ans, sizeof(int), 1)) < 0 && end == 0)
            {
                if (end != -1)
                {
                    ERR("mq_send");
                }
            }
        }

        //printf("sent to %s\n", name_c);

    }

    mq_close(qc);
    mq_close(qs);
    mq_close(qd);
    mq_close(qm);

    if (mq_unlink(name_s)){
    ERR("mq_unlink");
    }
    
    if (mq_unlink(name_d)){
        ERR("mq_unlink");
    }

    if (mq_unlink(name_m)){
        ERR("mq_unlink");
    }

    printf("\n");

    return EXIT_SUCCESS;
}