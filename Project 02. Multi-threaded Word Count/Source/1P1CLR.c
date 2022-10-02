#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

typedef struct sharedObject {
    FILE* readFile;
    int lineNum;
    char* readLine;
    pthread_mutex_t mutex;
    pthread_cond_t cv;
    bool isFull;
} so_t;

void* Producer(void* arg);
void* Consumer(void* arg);

int main(int argc, char* argv[]) {
    pthread_t prod[100];
    pthread_t cons[100];
    int prodNum, consNum;
    int rc;
    int* ret;
    int i;
    FILE* readFile;

    if (argc == 1) {
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(0);
    }
    so_t* so = malloc(sizeof(so_t));
    memset(so, 0, sizeof(so_t));

    // read file.
    readFile = fopen((char*)argv[1], "r");

    // handle file open exception.
    if (readFile == NULL) {
	    perror("readFile");
	    exit(EXIT_FAILURE);
    }
    so->readFile = readFile;
    so->readLine = NULL;

    // set the number of producer.
    if (argv[2] != NULL) {
	    prodNum = atoi(argv[2]);
	    if (prodNum > 100) prodNum = 100;
	    if (prodNum == 0) prodNum = 1;
    }
    else prodNum = 1;

    // set the number of consumer.
    if (argv[3] != NULL) {
	    consNum = atoi(argv[3]);
	    if (consNum > 100) consNum = 100;
	    if (consNum == 0) consNum = 1;
    }
    else consNum = 1;

    // initialize threads.
    pthread_mutex_init(&so->mutex, NULL);
    pthread_cond_init(&so->cv, NULL);

    // create threads.
    for (i = 0; i < prodNum; i++) {
	    pthread_create(&prod[i], NULL, Producer, so);
    }
    for (i = 0; i < consNum; i++) {
	    pthread_create(&cons[i], NULL, Consumer, so);
    }

    // join threads.
    printf("Main Continue...\n\n");
    for (i = 0; i < consNum; i++) {
	    rc = pthread_join(cons[i], (void**)&ret);
	    printf("main: consumer[%d] joined with %d\n", i, *ret);
    }
    for (i = 0; i < prodNum; i++) {
	    rc = pthread_join(prod[i], (void**)&ret);
	    printf("main: producer[%d] joined with %d\n", i, *ret);
    }
    pthread_exit(NULL);
    exit(EXIT_SUCCESS);
}

void* Producer(void* arg) {
    so_t* so = arg;
    FILE* readFile = so->readFile;
    char* readLine = NULL;
    int* ret = malloc(sizeof(int));
    int i = 0;
    size_t length = 0;
    ssize_t read = 0;

    while (true) {
	    // set pthread mutex, condition variable.
	    pthread_mutex_lock(&so->mutex);

	    // Mutex Lock Activated!
	    // wait for the condition that buffer is empty.

	    while (so->isFull == true) {
	        pthread_cond_wait(&so->cv, &so->mutex);
	    }

	    // now buffer is empty.
	    read = getdelim(&readLine, &length, '\n', readFile);
	
	    // handle end of file.
	    if (read == -1) {
	        so->isFull = true;
	        so->readLine = NULL;
	        pthread_cond_broadcast(&so->cv);
	        pthread_mutex_unlock(&so->mutex);
	        break;
	    }
	    so->lineNum = i++;

	    // share the line.
	    so->readLine = strdup(readLine);
	    so->isFull = true;
        pthread_cond_broadcast(&so->cv);
	    pthread_mutex_unlock(&so->mutex);
    }

    *ret = i;
    free(readLine);
    printf("\tProd: %d lines.\n", i);
    pthread_exit(ret);
}

void* Consumer(void* arg) {
    so_t* so = arg;
    int* ret = malloc(sizeof(int));
    int i = 0;
    int length;
    char* readLine;

    while (true) {
	    // set pthread mutex, condition variable.
	    pthread_mutex_lock(&so->mutex);
	
	    // wait for thd condition that buffer is full.
	    while (so->isFull == false) {
	        pthread_cond_wait(&so->cv, &so->mutex);
	    }
	
	    // now buffer is full.
	    readLine = so->readLine;

	    // handle end of file.
	    if (readLine == NULL) {
	        pthread_cond_broadcast(&so->cv);
	        pthread_mutex_unlock(&so->mutex);
	        break;
	    }
	
	    length = strlen(readLine);
	    printf("\tCons(%x): [%03d:%03d] %s", (unsigned int)pthread_self(), i, so->lineNum, readLine);
	    free(so->readLine);
	    i++;
        so->isFull = false;
        pthread_cond_broadcast(&so->cv);
        pthread_mutex_unlock(&so->mutex);
    }

    *ret = i;
    printf("\tCons: %d lines.\n\n", i);
    pthread_exit(ret);
}
