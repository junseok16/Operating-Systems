/*
 * class	: Operating System(MS)
 * Homework 02	: Multi-threaded Word Count; 1-producer n-consumer block read with statistic.
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

#define MAX_STRING_SIZE 30
#define ASCII_SIZE 256
#define BUFFER_SIZE 8
#define MUTEX_SIZE 8

int wordStat[MAX_STRING_SIZE];
int alphaStat[ASCII_SIZE];

typedef struct sharedObject {
    FILE* readFile;
    char* readLine[BUFFER_SIZE];
    bool isFull[BUFFER_SIZE];
    pthread_mutex_t mutex[MUTEX_SIZE];
    pthread_cond_t condVariable[MUTEX_SIZE];
    pthread_mutex_t setStat;
    pthread_mutex_t setIndex;
    int lineNum;
    int pIndex;
    int cIndex;
    int eIndex;
} so_t;

void* Producer(void* arg);
void* Consumer(void* arg);
void SetStat(char* arg);
void GetStat(void);

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
    memset(wordStat, 0, sizeof(wordStat));
    memset(alphaStat, 0, sizeof(alphaStat));

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

    // initialize mutex.    
    pthread_mutex_init(&so->setIndex, NULL);
    pthread_mutex_init(&so->setStat, NULL);
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

    // print statistic.
    GetStat();

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
*   arg: a pointer that pointes to the so structure.
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
*   arg: a pointer that pointes to the so structure.
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

        // collect statistic infomation from the block.
        pthread_mutex_lock(&so->setStat);
        SetStat(readLine);
        pthread_mutex_unlock(&so->setStat);

        lineIndex++;
        so->isFull[cin] = false;
        pthread_mutex_unlock(&so->mutex[cin]);
    }
    printf("\tConsumer reads %d blocks.\n", lineIndex);
    *ret = lineIndex;
    pthread_exit(ret);
}

/*
* void SetStat(char* arg)
*   This function collects the number of words and alphabets from block data.
*
* parameter: char*
*   arg: readLine pointer that points new block string.
*
* return: none.
*/

void SetStat(char* arg) {
    char* cptr = arg;
    char* substr = NULL;
    char* brka = NULL;
    char* sep = "{}()[],;\" \n\t^";
    size_t length = 0;

    for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
        if (*cptr == '\0') {
            break;
        }

        // count the numeber of words.
        length = strlen(substr);
        if (length >= 30) {
            length = 30;
        }
        wordStat[length - 1]++;

        // count the number of alphabet.
        cptr = substr;

        length = strlen(substr);
        for (int i = 0; i < length + 1; i++) {
            if (*cptr < 256 && *cptr > 1) {
                alphaStat[*cptr]++;
            }
            cptr++;
        }
    }
    return;
}

/*
* void GetStat(void)
*   This function shows the statistics that contain the number of words and alphabets in the terminal.
*
* parameter: none.
*
* return: none.
*/

void GetStat(void) {
    int sum = 0;
    for (int i = 0; i < 30; i++) {
        sum += wordStat[i];
    }

    // print word distributions.
    printf("\n\t〓〓〓〓〓〓〓 WORD DISTRIBUTION 〓〓〓〓〓〓〓\n");
    printf("\t  #ch  freq \n");
    for (int i = 0; i < 30; i++) {
        int num_star = wordStat[i] * 80 / sum;
        printf("\t[%03d]: %4d \t", i + 1, wordStat[i]);

        for (int j = 0; j < num_star; j++)
            printf("■");
        printf("\n");
    }

    // print alphabet list.
    printf("\n\t〓〓〓〓〓〓〓 ALPHABET LIST 〓〓〓〓〓〓〓\n");
    printf("\tA(a) : %8d | \tN(n) : %8d\n", alphaStat['A'] + alphaStat['a'], alphaStat['N'] + alphaStat['n']);
    printf("\tB(b) : %8d | \tO(o) : %8d\n", alphaStat['B'] + alphaStat['b'], alphaStat['O'] + alphaStat['o']);
    printf("\tC(c) : %8d | \tP(p) : %8d\n", alphaStat['C'] + alphaStat['c'], alphaStat['P'] + alphaStat['p']);
    printf("\tD(d) : %8d | \tQ(q) : %8d\n", alphaStat['D'] + alphaStat['d'], alphaStat['Q'] + alphaStat['q']);
    printf("\tE(e) : %8d | \tR(r) : %8d\n", alphaStat['E'] + alphaStat['e'], alphaStat['R'] + alphaStat['r']);
    printf("\tF(f) : %8d | \tS(s) : %8d\n", alphaStat['F'] + alphaStat['f'], alphaStat['S'] + alphaStat['s']);
    printf("\tG(g) : %8d | \tT(t) : %8d\n", alphaStat['G'] + alphaStat['g'], alphaStat['T'] + alphaStat['t']);
    printf("\tH(h) : %8d | \tU(u) : %8d\n", alphaStat['H'] + alphaStat['h'], alphaStat['U'] + alphaStat['u']);
    printf("\tI(i) : %8d | \tV(v) : %8d\n", alphaStat['I'] + alphaStat['i'], alphaStat['V'] + alphaStat['v']);
    printf("\tJ(j) : %8d | \tW(w) : %8d\n", alphaStat['J'] + alphaStat['j'], alphaStat['W'] + alphaStat['w']);
    printf("\tK(k) : %8d | \tX(x) : %8d\n", alphaStat['K'] + alphaStat['k'], alphaStat['X'] + alphaStat['x']);
    printf("\tL(l) : %8d | \tY(y) : %8d\n", alphaStat['L'] + alphaStat['l'], alphaStat['Y'] + alphaStat['y']);
    printf("\tM(m) : %8d | \tZ(z) : %8d\n", alphaStat['M'] + alphaStat['m'], alphaStat['Z'] + alphaStat['z']);
    printf("\n");
    return;
}
