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
}thread_args_t;

void ReadArguments(int argc, char **argv, int *n, int *k);
void init_arr(double ** arr, int k);
void fill_arr_rand(double * arr, int k);
void print_arr(double *arr, int k);
void *thread_work(void*);


int main(int argc, char ** argv){
    int n, k;

    ReadArguments(argc, argv, &n, &k);

    srand(getpid() * time(NULL));

    double* arr = NULL; //malloc(k*sizeof(double));

    init_arr(&arr, k);
    fill_arr_rand(arr, k);
    print_arr(arr, k);

    pthread_mutex_t mx_arr= PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutex_t* mx_arr = (pthread_mutex_t*)malloc(n*sizeof(pthread_mutex_t));
    // for (int i = 0; i < n; i++)
    // {
    //     if(0 != pthread_mutex_init(&mx_arr[i],NULL)){
    //         ERR("pthread_mutex_init");
    //     }
    // }
    

    thread_args_t* threads = (thread_args_t*)malloc(n*sizeof(thread_args_t));

    for (int i = 0; i < n; i++)
    {
        threads[i].seed = rand();
        threads[i].arr = arr;
        threads[i].k=k;
        threads[i].mx_arr=&mx_arr;
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
    free(threads);
    //free(mx_arr);
    return EXIT_SUCCESS;
}

void init_arr(double **arr, int k){
    *arr=NULL;
    *arr = (double*)malloc(k*sizeof(double));

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

    int ind = rand_r(&args->seed) % (args->k);
    pthread_mutex_lock(args->mx_arr);
    args->arr[ind]=sqrt(args->arr[ind]);
    printf("%lf \n", args->arr[ind]);
    pthread_mutex_unlock(args->mx_arr);
    //printf("%d\n", rand_r(&args->seed)%args->k);
    return NULL;
}