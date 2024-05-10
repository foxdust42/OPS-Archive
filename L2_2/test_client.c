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

void usage(void)
{
	fprintf(stderr, "Pass pid of a process\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv){

    pid_t server_pid;
    if (argc != 2)
    {
        usage();
    }
    if ((server_pid = (pid_t)atoi(argv[1])) == 0)
    {
        usage();
    }
    
    pid_t pid = getpid();

    char name_s[15+2]; 
    char name_d[15+2]; 
    char name_m[15+2]; 
    char name[15]; 

    sprintf(name, "/%d", pid);
    sprintf(name_s, "/%d_s", server_pid);
    sprintf(name_d, "/%d_d", server_pid);
    sprintf(name_m, "/%d_m", server_pid);


    mqd_t qid, qs, qd, qm;

    struct mq_attr attr;

    attr.mq_maxmsg = 5;
    attr.mq_msgsize = sizeof(int);


    if (TEMP_FAILURE_RETRY((qs = mq_open(name_s, O_RDWR ))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
    if (TEMP_FAILURE_RETRY((qd = mq_open(name_d, O_RDWR ))) == (mqd_t)-1)
    {
        mq_close(qs);
        ERR("mq_open");
    }
    if (TEMP_FAILURE_RETRY((qm = mq_open(name_m, O_RDWR ))) == (mqd_t)-1)
    {
        mq_close(qs);
        mq_close(qd);
        ERR("mq_open");
    }
    if (TEMP_FAILURE_RETRY((qid = mq_open(name, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
    {
        mq_close(qs);
        mq_close(qd);
        mq_close(qm);
        ERR("mq_open");
    }
    printf("all open\n");
    //sleep(1);
    int num1 = 0, num2 = 0, return_val;
    struct message msg;
    msg.client_pid = pid;

    printf("%d\n", pid);

    struct timespec ts;

    while (1){

        scanf("%d %d", &num1, &num2);
        msg.num1 = num1;
        msg.num2 = num2;
        printf("%d %d\n", num1, num2);

        if (feof(stdin))
        {
            msg.client_pid = -1;
            msg.num1=0;
            msg.num2=0;
            if (TEMP_FAILURE_RETRY(mq_send(qs, (const char *)&msg, sizeof(struct message), 1)) < 0)
            {
                ERR("mq_send");
            }
            break;
        }
        msg.client_pid = pid;
        printf("%d - %d - %d\n" , msg.client_pid, msg.num1, msg.num2);

        if (TEMP_FAILURE_RETRY(mq_send(qs, (const char *)&msg, sizeof(struct message), 1)) < 0)
        {
            ERR("mq_send");
        }
        printf("sent\n");

        if (clock_gettime(CLOCK_REALTIME, &ts))
        {
            ERR("clock_gettime");
        }
        ts.tv_nsec+=100000000;        

        if (TEMP_FAILURE_RETRY(mq_timedreceive(qid, (char*)&return_val, sizeof(int), NULL, &ts)) < 0)
        {
            if (errno ==ETIMEDOUT)
            {
                msg.client_pid = -1;
                msg.num1=0;
                msg.num2=0;
                if (TEMP_FAILURE_RETRY(mq_send(qs, (const char *)&msg, sizeof(struct message), 1)) < 0)
                {
                    ERR("mq_send");
                }
            }
            else{
                ERR("mq_recieve");
            }
        }
        printf("recieved %d\n", return_val);
        if (TEMP_FAILURE_RETRY(mq_send(qd, (const char *)&msg, sizeof(struct message), 1)) < 0)
        {
            ERR("mq_send");
        }
        printf("sent\n");

        if (clock_gettime(CLOCK_REALTIME, &ts))
        {
            ERR("clock_gettime");
        }
        ts.tv_nsec+=100000000;  

        if (TEMP_FAILURE_RETRY(mq_timedreceive(qid, (char*)&return_val, sizeof(int), NULL, &ts)) < 0)
        {
            if (errno ==ETIMEDOUT)
            {
                msg.client_pid = -1;
                msg.num1=0;
                msg.num2=0;
                if (TEMP_FAILURE_RETRY(mq_send(qs, (const char *)&msg, sizeof(struct message), 1)) < 0)
                {
                    ERR("mq_send");
                }
            }
            else{
                ERR("mq_recieve");
            }
        }
        printf("recieved %d\n", return_val);
        if (TEMP_FAILURE_RETRY(mq_send(qm, (const char *)&msg, sizeof(struct message), 1)) < 0)
        {
            ERR("mq_send");
        }
        printf("sent\n");

        if (clock_gettime(CLOCK_REALTIME, &ts))
        {
            ERR("clock_gettime");
        }
        ts.tv_nsec+=100000000;  

        if (TEMP_FAILURE_RETRY(mq_timedreceive(qid, (char*)&return_val, sizeof(int), NULL, &ts)) < 0)
        {
            if (errno ==ETIMEDOUT)
            {
                msg.client_pid = -1;
                msg.num1=0;
                msg.num2=0;
                if (TEMP_FAILURE_RETRY(mq_send(qs, (const char *)&msg, sizeof(struct message), 1)) < 0)
                {
                    ERR("mq_send");
                }
            }
            else{
                ERR("mq_recieve");
            }
        }
        printf("recieved %d\n", return_val);
        
    }


    mq_close(qid);
    mq_close(qs);
    mq_close(qd);
    mq_close(qm);

    if (mq_unlink(name)){
        ERR("mq_unlink");
    }
    
    return EXIT_SUCCESS;
}