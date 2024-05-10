#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <mqueue.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define CHAMP_LIMIT 4
#define BACKLOG 3
#define COURT_LIMIT 2
/* condition */
volatile sig_atomic_t work = 1;

typedef struct {
	int *id_pass;
	int socket;
    pthread_barrier_t* barrier;
    pthread_cond_t *cond;
	pthread_mutex_t *mutex;
} thread_arg;

void siginthandler(int sig)
{
	work = 0;
}

void sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_handler = f;

	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

int make_socket(int domain, int type)
{
	int sock;
	sock = socket(domain, type, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
}

int bind_tcp_socket(uint16_t port)
{
	struct sockaddr_in addr;
	int socketfd, t = 1;
	socketfd = make_socket(PF_INET, SOCK_STREAM);
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
		ERR("setsockopt");
	if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR("bind");
	if (listen(socketfd, BACKLOG) < 0)
		ERR("listen");
	return socketfd;
}


void* threadfunc(void* arg){
    thread_arg args;
    memcpy(&args, arg, sizeof(args));
    int id;
    pthread_barrier_wait(args.barrier);

    printf("%ld \n", pthread_self());

    pthread_mutex_lock(args.mutex);
    while (*args.id_pass == 0)
    {
        pthread_cond_wait(args.cond, args.mutex);
    }
    
    id = *args.id_pass;
    *args.id_pass = 0;

    pthread_cond_broadcast(args.cond);
    pthread_mutex_unlock(args.mutex);

    pthread_barrier_wait(args.barrier);

    printf("%d\n", id);

    char msg[1];

    send(args.socket, msg, 1, 0);
    

    close(args.socket);

    return EXIT_SUCCESS;
}

void init(thread_arg t_args[CHAMP_LIMIT], int socket, pthread_barrier_t *barrier, pthread_mutex_t *mutex, pthread_cond_t *cond,
     pthread_t threads[CHAMP_LIMIT], int * id_pass ){
    int con_fd;
    for (int i = 0; i < CHAMP_LIMIT; i++)
    {
        if (listen(socket, BACKLOG)<0)
        {
            ERR("listen");
        }
        while (1==1)
        {
            if ((con_fd = TEMP_FAILURE_RETRY(accept(socket, NULL, NULL))) < 0) {
		    if (EAGAIN == errno || EWOULDBLOCK == errno)
			    continue;
		    ERR("accept");
	        }
            break;
        }
        
        t_args[i].barrier = barrier;
        t_args[i].cond = cond;
        t_args[i].mutex = mutex;
        t_args[i].socket = con_fd;
        t_args[i].id_pass = id_pass;

        if (pthread_create(&threads[i], NULL, *threadfunc, (void*)&t_args[i] )< 0)
        {
            ERR("pthread_create");
        }
        pthread_detach(threads[i]);
        
    }
}

void init_courts(pthread_t courts[COURT_LIMIT]){

}

void assign_id(int * id, pthread_mutex_t *mutex, pthread_cond_t * cond){

    for (int i = 0; i < CHAMP_LIMIT; i++)
    {
        pthread_mutex_lock(mutex);
        *id = i+1;
        pthread_cond_broadcast(cond);
            while (*id != 0)
            {
                pthread_cond_wait(cond, mutex);
            }
            
        pthread_mutex_unlock(mutex);
    }
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char ** argv){
    int id_pass;
    pthread_t threads[CHAMP_LIMIT];
    pthread_t courts[COURT_LIMIT];
    thread_arg t_args[CHAMP_LIMIT];
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_barrier_t barrier;
    int socket;
    sigset_t mask, oldmask;
   	if (argc != 2)
	    usage(argv[0]);
    id_pass = 0;
    srand(getpid() * time(NULL));
    sethandler(SIG_IGN, SIGPIPE);
	sethandler(siginthandler, SIGINT);
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	socket = bind_tcp_socket(atoi(argv[1]));

    mqd_t queue;
    struct mq_attr q_attr;
    

    int new_flags = fcntl(socket, F_GETFL) | O_NONBLOCK;
    if (fcntl(socket, F_SETFL, new_flags) == -1)
		ERR("fcntl");

    if (pthread_barrier_init(&barrier, NULL, CHAMP_LIMIT+1) < 0)
    {
        ERR("pthread_barrier_init");
    }

    init(t_args, socket, &barrier, &mutex, &cond, threads, &id_pass);
    
    pthread_barrier_wait(&barrier); //Championship start

    assign_id(&id_pass, &mutex, &cond);

    pthread_barrier_wait(&barrier);
    close(socket);
    return EXIT_SUCCESS;
}