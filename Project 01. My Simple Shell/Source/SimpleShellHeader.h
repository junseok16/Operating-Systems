#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_SIZE 512
#define CONTINUE_SHELL 1

void openSh(void);
void closeSh(void);
void printPrompt(void);

int readCmd(char cmd[], int size);
void parseCmd(char* newCmd[], char cmd[]);
int execBuiltIn(char* newCmd[]);

void readPath(char* newPath[], char path[]);
void execProcess(char* newPath[], char* newCmd[]);
void execObj(char* newCmd[], char** environ);
void execCmd(char* newPath[], char* newCmd[], char** environ);
 
char histCmd[MAX_SIZE][MAX_SIZE];
unsigned int histIndex;
