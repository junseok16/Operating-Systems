/*
 * class	: Operating System(MS)
 * Homework 02	: Multi-threaded Word Count; 1-producer n-consumer block read without statistic.
 * Author	: junseok Tak
 * Student ID	: 32164809
 * Date		: 2021-10-25
 * Professor	: seehwan Yoo
 * Left freedays: 4
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 8
#define MUTEX_SIZE 8

typedef struct sharedObject {
    FILE* readFile;
    char* readLine[BUFFER_SIZE];
    bool isFull[BUFFER_SIZE];
    pthread_mutex_t mutex[MUTEX_SIZE];
    pthread_cond_t condVariable[MUTEX_SIZE];
    pthread_mutex_t setIndex;
    int lineNum;
    int pIndex;
    int cIndex;
    int eIndex;
} so_t;

void* Producer(void* arg);
void* Consumer(void* arg);

int main(int argc, char* argv[]) {
    pthread_t prod[100];
    pthread_t cons[100];
    int PRODUCER_NUMBER, CONSUMER_NUMBER;
    int rc;
    int* ret;
    FILE* readFile;

    if (argc == 1) {
        printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
        exit(EXIT_SUCCESS);
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

    // set the number of producer.
    if (argv[2] != NULL) {
        PRODUCER_NUMBER = atoi(argv[2]);
        if (PRODUCER_NUMBER > 100) PRODUCER_NUMBER = 1;
        if (PRODUCER_NUMBER == 0) PRODUCER_NUMBER = 1;
    }
    else PRODUCER_NUMBER = 1;

    // set the number of consumer.
    if (argv[3] != NULL) {
        CONSUMER_NUMBER = atoi(argv[3]);
        if (CONSUMER_NUMBER > 100) CONSUMER_NUMBER = 100;
        if (CONSUMER_NUMBER == 0) CONSUMER_NUMBER = 1;
    }
    else CONSUMER_NUMBER = 1;

    // initialize mutex, condition variable
    pthread_mutex_init(&so->setIndex, NULL);
    for (int innerLoopIndex = 0; innerLoopIndex < MUTEX_SIZE; innerLoopIndex++) {
        pthread_mutex_init(&so->mutex[innerLoopIndex], NULL);
        pthread_cond_init(&so->condVariable[innerLoopIndex], NULL);
    }

    // create threads.
    for (int i = 0; i < PRODUCER_NUMBER; i++) {
        pthread_create(&prod[i], NULL, Producer, so);
    }
    for (int i = 0; i < CONSUMER_NUMBER; i++) {
        pthread_create(&cons[i], NULL, Consumer, so);
    }

    sleep(0);
    // join threads.
    printf("\t〓〓〓〓〓〓〓 STATISTIC REPORT 〓〓〓〓〓〓〓\n");
    for (int i = 0; i < PRODUCER_NUMBER; i++) {
        rc = pthread_join(prod[i], (void**)&ret);
        printf("\tProducer No.%d joined with %d blocks.\n", i, *ret);
    }
    for (int i = 0; i < CONSUMER_NUMBER; i++) {
        rc = pthread_join(cons[i], (void**)&ret);
        printf("\tConsumer No.%d joined with %d blocks.\n", i, *ret);
    }

    // terminate threads.
    pthread_exit(NULL);
    exit(EXIT_SUCCESS);
}

/*
* void* Producer(void* arg);
*   This function serves as a producer. This function finds an empty buffer, reads a line from a given file, and stores it in that buffer.
*   Mutual exclusion is guaranteed while data is stored in the buffer, and condition variable signals are broadcast to consumers when the buffer is full.
*
* parameter: void*
*   arg: A pointer that pointes to the so structure.
*
* return: none.
*/

void* Producer(void* arg) {
    so_t* so = arg;
    FILE* readFile = so->readFile;
    char readLine[4096];
    int* ret = malloc(sizeof(int));
    int lineIndex = 0;
    int pin = 0;
    size_t length = 0;
    ssize_t read = 0;

    while (true) {
        // set new buffer index, then mutex lock.
        pthread_mutex_lock(&so->setIndex);
        pin = so->pIndex;
        so->pIndex = (pin + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&so->setIndex);
        pthread_mutex_lock(&so->mutex[pin]);

        // search an empty buffer.
        while (so->isFull[pin] == true) {
            pthread_mutex_unlock(&so->mutex[pin]);
            pin = (pin + 1) % BUFFER_SIZE;
            pthread_mutex_lock(&so->mutex[pin]);
        }

        // read a single block.
        read = fread(readLine, sizeof(readLine), 1, readFile);
        sleep(0);

        // put a block into the buffer.
        so->lineNum = lineIndex++;
        so->readLine[pin] = strdup(readLine);
        so->isFull[pin] = true;
        memset(readLine, '\0', sizeof(readLine));
        pthread_cond_broadcast(&so->condVariable[pin]);
        pthread_mutex_unlock(&so->mutex[pin]);

        // handle eof
        if (read == 0) {
            // set end index to true.
            pthread_mutex_lock(&so->setIndex);
            so->eIndex = 1;
            pthread_mutex_unlock(&so->setIndex);

            for (int innerLoopIndex = 0; innerLoopIndex < BUFFER_SIZE; innerLoopIndex++) {
                pthread_mutex_lock(&so->mutex[innerLoopIndex]);

                if (so->isFull[innerLoopIndex] == false) {
                    so->isFull[innerLoopIndex] = true;
                    so->readLine[innerLoopIndex] = NULL;
                    pthread_cond_broadcast(&so->condVariable[innerLoopIndex]);
                }
                pthread_mutex_unlock(&so->mutex[innerLoopIndex]);
            }
            break;
        }
    }
    printf("\tProducer reads %d blocks.\n", lineIndex);
    *ret = lineIndex;
    pthread_exit(ret);
}

/*
* void* Consumer(void* arg);
*   This function serves as a consumer. This function waits in an empty buffer until the producer broadcasts a signal to the condition variable.
*   When the buffer is full, read the data stored in the buffer.
*
* parameter: void*
*   arg: A pointer that pointes to the so structure.
*
* return: none.
*/

void* Consumer(void* arg) {
    so_t* so = arg;
    char* readLine;
    int* ret = malloc(sizeof(int));
    int lineIndex = 0;
    int cin = 0;

    while (true) {
        // set new buffer index, then mutex lock.
        pthread_mutex_lock(&so->setIndex);
        cin = so->cIndex;
        so->cIndex = (so->cIndex + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&so->setIndex);
        pthread_mutex_lock(&so->mutex[cin]);

        // wait for thread condition that buffer is full.
        while (so->isFull[cin] == false && so->eIndex == 0) {
            pthread_cond_wait(&so->condVariable[cin], &so->mutex[cin]);
        }

        // get the block from buffer.
        readLine = so->readLine[cin];

        // handle end of file.
        if (readLine == NULL) {
            pthread_mutex_unlock(&so->mutex[cin]);
            break;
        }

        lineIndex++;
        so->isFull[cin] = false;
        pthread_mutex_unlock(&so->mutex[cin]);
    }
    printf("\tConsumer reads %d blocks.\n", lineIndex);
    *ret = lineIndex;
    pthread_exit(ret);
}
