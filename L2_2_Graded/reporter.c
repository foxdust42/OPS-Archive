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

void open_queue(mqd_t * qid, char * name){
    
    if (TEMP_FAILURE_RETRY(( *qid = mq_open(name, O_RDWR | O_NONBLOCK))) == (mqd_t)-1)
    {
        if (errno == ENOENT)
        {
            printf("No such queue exists, terminating...\n");
            exit(EXIT_FAILURE);
        }
        
        ERR("mq_open");
    }
}

void check_input(int argc, char ** argv){
    if (argc != 2)
    {
        printf("Improper input\n");
        exit(EXIT_FAILURE);
    }
}

//https://stackoverflow.com/questions/65542677/reading-in-exactly-n-bytes-from-standard-input-in-c
int read_from_stdin(char message[12]){
    char c;
    for (int i = 0; i < 12; i++)
    {
        message[i] = 0;
    }
    for (int i = 0; i < 12; i++)
    {
        c = getc(stdin);
        if (c == EOF || c == '\n')
        {
            return -1;
        }
        message[i] = c;
    }
    //clear stdin
    while((c = getc(stdin)) != '\n' && c != EOF);
    return 0;
}

void print_buffer(char * message, int len){
    for (int i = 0; i < len; i++)
    {
        printf("%c", message[i]);
    }
    printf("\n");
}

int send_message(mqd_t qid, char * message, int len){

    if (TEMP_FAILURE_RETRY(mq_send(qid, message, len, 1)) < 0)
    {
        if (errno == EAGAIN)
        {
            return -1;
        }
        ERR("mq_send");
    }
    return 0;
}

int main(int argc, char ** argv){
    mqd_t qid = -1;
    char message[12]; 
    check_input(argc, argv);
    open_queue(&qid, argv[1]);
    //printf("%s", argv[1]);
    printf("Connection successful\n");
    //printf("%d\n", qid);
    int lines = 5;
    while(lines--){
        read_from_stdin(message);
        printf("Sending to editor: ");
        print_buffer(message, 12);
        printf("\n");
        if(send_message(qid, message, 12) == -1 ){
            printf("I am going to antoher newspaper!\n");
            lines=0;
        }
    }

    mq_close(qid);
    return EXIT_SUCCESS;
}