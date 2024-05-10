#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define FIFO_1 "p1.fifo"
#define FIFO_2 "p2.fifo"

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s fifo_file\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char ** argv){
    
}