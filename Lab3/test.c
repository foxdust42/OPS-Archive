#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define INTERVAL 100 //ms
#define MAX_RAND_FLOAT 60.0

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct thread_args{
    pthread_t tid;
    unsigned int seed;
    double* arr;
    pthread_mutex_t* mx_arr;
    int k;
    int *remaining;
    pthread_mutex_t* mx_remaining;
    int *visited;
    pthread_mutex_t* mx_visited;
}thread_args_t;

void ReadArguments(int argc, char **argv, int *n, int *k);
void init_arr_d(double ** arr, int k);
void init_arr_i(int ** arr, int k);
void fill_arr_rand(double * arr, int k);
void fill_arr_zero(int *arr, int k);
void print_arr(double *arr, int k);
void *thread_work(void*);

int main(int argc, char ** argv){
    int n, k;
    ReadArguments(argc, argv, &n, &k);
    int remaining = k;

    srand(getpid() * time(NULL));

    double* arr = NULL; //malloc(k*sizeof(double));
    int * visited = NULL;

    init_arr_d(&arr, k);
    fill_arr_rand(arr, k);
    print_arr(arr, k);

    init_arr_i(&visited, k);
    fill_arr_zero(visited, k);

    pthread_mutex_t mx_remaining = PTHREAD_MUTEX_INITIALIZER;


    pthread_mutex_t* mx_arr = (pthread_mutex_t*)malloc(n*sizeof(pthread_mutex_t));
    for (int i = 0; i < n; i++)
    {
        if(0 != pthread_mutex_init(&mx_arr[i],NULL)){
            ERR("pthread_mutex_init");
        }
    }
    

    pthread_mutex_t* mx_visited = (pthread_mutex_t*)malloc(n*sizeof(pthread_mutex_t));
    for (int i = 0; i < n; i++)
    {
        if(0 != pthread_mutex_init(&mx_visited[i],NULL)){
            ERR("pthread_mutex_init");
        }
    }

    thread_args_t* threads = (thread_args_t*)malloc(n*sizeof(thread_args_t));

    for (int i = 0; i < n; i++)
    {
        threads[i].seed = rand();
        threads[i].arr = arr;
        threads[i].k=k;
        threads[i].mx_arr=mx_arr;
        threads[i].visited = visited;
        threads[i].mx_visited = mx_visited;
        threads[i].remaining = &remaining;
        threads[i].mx_remaining = &mx_remaining;
        if (0 != pthread_create(&threads[i].tid, NULL, thread_work, &threads[i]))
        {
            ERR("pthread_create");
        }
    }
    
    for (int i = 0; i < n; i++)
    {
        if (0 != pthread_join(threads[i].tid, NULL))
        {
            ERR("pthread_join");
        }
    }
    
    print_arr(arr, k);

    free(arr);
    free(visited);
    free(threads);
    free(mx_arr);
    free(mx_visited);
    return EXIT_SUCCESS;
}

void init_arr_d(double **arr, int k){
    *arr=NULL;
    *arr = (double*)malloc(k*sizeof(double));

    if (*arr == NULL)
    {
        ERR("malloc");
    }
    return;
}
void init_arr_i(int **arr, int k){
    *arr=NULL;
    *arr = (int*)malloc(k*sizeof(int));

    if (*arr == NULL)
    {
        ERR("malloc");
    }
    return;
}

void fill_arr_rand(double *arr, int k){
    for (int i = 0; i < k; i++)
    {
        arr[i] = ((double)rand() / (double)RAND_MAX) * MAX_RAND_FLOAT;
    }
}

void fill_arr_zero(int *arr, int k){
    for (int i = 0; i < k; i++)
    {
        arr[i] = 0;
    }
}

void print_arr(double* arr, int k){
    printf("[ ");
    for (int i = 0; i < k; i++)
    {
        printf("%lf ", arr[i]);
    }
    printf("]\n");
}

void ReadArguments(int argc, char **argv, int *n, int *k)
{
	*n = 3;
	*k = 7;

	if (argc >= 2) {
		*n = atoi(argv[1]);
		if (*n <= 0) {
			printf("Invalid value for 'n'");
			exit(EXIT_FAILURE);
		}
	}
	if (argc >= 3) {
		*k = atoi(argv[2]);
		if (*k <= 0) {
			printf("Invalid value for 'k'");
			exit(EXIT_FAILURE);
		}
	}
}

void *thread_work(void* in){
    thread_args_t* args = (thread_args_t*)in;
    struct timespec slp = { 0, INTERVAL * 1000000L };
    int ind = rand_r(&args->seed) % (args->k);

    while (1)
    {
        slp.tv_nsec = INTERVAL * 1000000L;
        slp.tv_sec = 0;
        while (1)
        {
            pthread_mutex_lock(args->mx_remaining);
                if (*args->remaining == 0)
                {
                    pthread_mutex_unlock(args->mx_remaining);
                    return NULL;
                }
            pthread_mutex_unlock(args->mx_remaining);

            ind = rand_r(&args->seed) % (args->k);
            pthread_mutex_lock(args->mx_visited+sizeof(pthread_mutex_t)*ind);
                if (args->visited[ind]==0)
                {
                    args->visited[ind]=1;
                    pthread_mutex_unlock(args->mx_visited+sizeof(pthread_mutex_t)*ind);
                    break;
                }
                
            pthread_mutex_unlock(args->mx_visited+sizeof(pthread_mutex_t)*ind);
        }

        pthread_mutex_lock(args->mx_arr+sizeof(pthread_mutex_t)*ind);
        args->arr[ind]=sqrt(args->arr[ind]);
        printf("%lf \n", args->arr[ind]);
        pthread_mutex_unlock(args->mx_arr+sizeof(pthread_mutex_t)*ind);
        pthread_mutex_lock(args->mx_remaining);
        *args->remaining-=1;
        pthread_mutex_unlock(args->mx_remaining);
        nanosleep(&slp, &slp);
    }
}