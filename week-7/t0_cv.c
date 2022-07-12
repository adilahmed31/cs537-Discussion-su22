#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"


void *firstthread(void *arg) {
    printf("this runs first\n");
    return NULL;
}

void *secondthread(void *arg) {
    printf("this runs second\n");
    return NULL;
}

int main(int argc, char *argv[]) {                    
    if (argc != 1) {
	fprintf(stderr, "usage: main\n");
	exit(1);
    }

    pthread_t p1, p2;
    printf("main: begin\n");
    Pthread_create(&p1, NULL, firstthread, NULL); 
    Pthread_create(&p2, NULL, secondthread, NULL);
    // join waits for the threads to finish
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: end\n");
    return 0;
}

