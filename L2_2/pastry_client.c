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

void add_request(mqd_t request_mq){
    char spec_cookie[2] = {0, 0};
    if (TEMP_FAILURE_RETRY(mq_send(request_mq, spec_cookie, 2, 1)) < 0)
    {
        if (errno == EAGAIN)
        {
            return;
        }
        
        ERR("mq_send");
    }
}

int main(int argc, char ** argv){
    char request_queue_name[15] = "/request_queue"; 
    char cookie_queue_name[14] = "/cookie_queue";

    int fail_count = 0, fail_tmp = 0;

    mqd_t request_mq, cookie_mq;

    char cookie[2];
    
    pid_t pid = getpid();

    sleep(1);

    if (TEMP_FAILURE_RETRY((request_mq = mq_open(request_queue_name, O_WRONLY | O_NONBLOCK))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
    if (TEMP_FAILURE_RETRY((cookie_mq = mq_open(cookie_queue_name, O_RDONLY | O_NONBLOCK))) == (mqd_t)-1)
    {
        ERR("mq_open");
    }
    
    while (fail_count < 3)
    {
        if (TEMP_FAILURE_RETRY(mq_receive(cookie_mq, cookie, 2, NULL)) < 0 )
        {
            if (errno == EAGAIN)
            {
                printf("[%d] Nichts für mich\n", pid);
                add_request(request_mq);
                fail_count++;
                fail_tmp = 1;
            }
            else{
                ERR("mq_recieve");
            }
        }
        if (!fail_tmp)
        {
            printf("[%d] got %d, %d\n", pid, cookie[0], cookie[1]);
            fail_count = 0;
        }
        fail_tmp=0;

        sleep(1);
    }

    printf("[%d] Auf wiedersehen\n", pid);

    mq_close(request_mq);
    mq_close(cookie_mq);

    return EXIT_SUCCESS;
}