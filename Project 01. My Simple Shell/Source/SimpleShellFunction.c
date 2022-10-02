#include "SimpleShellHeader.h"

/*
 * void openSh(void);
 *	This function prints the opening screen of my simple shell.
 *
 * parameter:
 *	none.
 * return:
 *	none.
 */
void openSh(void) {
    printf("\n\n\n\x1b[33m");
    printf("\t███╗   ███╗██╗   ██╗███████╗██╗███████╗██╗  ██╗\n");
    printf("\t████╗ ████║╚██╗ ██╔╝██╔════╝██║██╔════╝██║  ██║\n");
    printf("\t██╔████╔██║ ╚████╔╝ ███████╗██║███████╗███████║\n");
    printf("\t██║╚██╔╝██║  ╚██╔╝  ╚════██║██║╚════██║██╔══██║\n");
    printf("\t██║ ╚═╝ ██║   ██║   ███████║██║███████║██║  ██║\n");
    printf("\t╚═╝     ╚═╝   ╚═╝   ╚══════╝╚═╝╚══════╝╚═╝  ╚═╝ by junseok16\n");
    printf("\n");
    printf("\tNow starting...\n\n");
    printf("\x1b[0m");
    sleep(1);
    return;
}

/*
 * void closeSh(void);
 *	This function prints the closeing screen of my simple shell.
 *
 * parameter:
 *	none.
 * return:
 *	none.
 */
void closeSh(void) {
    printf("\n\x1b[33m");
    printf("\tNow Shutting down...\n\n");
    printf("\x1b[0m");
    sleep(1);
    return;
}

/*
 * void printDir(void);
 *	This function prints user's current directory.
 *
 * parameter:
 *	none.
 * return:
 *	none.
 * exception:
 *	if getcwd() fails to find current directory, throw EXIT_FAILURE and terminate program.
 */

void printPrompt(void) {
    char curDir[512];
    time_t t;
    char curTime[128];

    if (getcwd(curDir, 512) == NULL) {
        perror("getcwd error.");
        exit(EXIT_FAILURE);
    }

    time(&t);
    strftime(curTime, 128, "%Y %B %dth %p %I:%M", localtime(&t));
    printf("\x1b[33m%s \x1b[0m", curTime);
    printf("%s@assam:~", getenv("USER"));
    printf("%s$ ", curDir);
    return;
}

/*
 * int readCmd(char cmd[], int limit);
 *	This function reads a raw instruction entered by user,
 *	and eleminates  unnecessary spaces(' ').
 *
 * parameter: char, int
 *	cmd[]	    : User's raw instruction.
 *	limit	    : Maximum length of user's raw instruction  which is 512.
 *
 * return: int
 *	return 1    : User inputs nothing, so there is no instruction.
 *	return 0    : User inputs a instruction, and readCmd() works well.
 */

int readCmd(char cmd[], int size) {
    int ch, i = 0;

    // remove unnecesary space(' ') from user's instruction.
    while ((ch = getchar()) != '\n') {
	if (i < size && (!isspace(ch) || ((i >= 1) && cmd[i - 1] != ' ')))
	    cmd[i++] = ch;
    }

    while (i > 0 && cmd[i - 1] == ' ')
	i--;

    cmd[i] = '\0';

    // handle '\n' case.
    if (i == 0)
	// return CONTINUE_SHELL, which is 1.
	return CONTINUE_SHELL;

    // store instruction for history.
    strcpy(histCmd[histIndex++], cmd);
    if (histIndex >= MAX_SIZE)
	histIndex = 0;

    return 0;
}

/*
 * void parseCmd(char* newCmd[], char cmd[]);
 *	This function separates a command and arguments in the instruction,
 *	and stores them in newCmd[] array.
 *
 * parameters: char*, char
 *	*newCmd[]   : Array that stores a command and arguments.
 *	cmd[]	    : Instruction without unnecessary spaces(' ').
 *
 * return:
 *	none.
 */
void parseCmd(char* newCmd[], char cmd[]) {
    int innerLoopIndex = 1;
    char* savePtr = NULL;

    // newCmd[0] for first command.
    newCmd[0] = strtok_r(cmd, " ", &savePtr);

    // newCmd[1], newCmd[2], ... for arguments.
    while (1) {
	newCmd[innerLoopIndex] = strtok_r(NULL, " ", &savePtr);
	if (newCmd[innerLoopIndex] == NULL)
	    break;
	innerLoopIndex++;
    }
    return;
}

/*
 * int execBuiltIn(char* newCmd[]);
 *	This function excutes "history", "exit", "quit", "cd" command.
 *
 * argument: char*
 *      *newCmd[]   : Array that stores user instruction
 *	
 * return: int
 *      return 1    : After "history", "cd" execution, restart shell again.
 *	return 0    : User command is not builtIn command.
 */
int execBuiltIn(char* newCmd[]) {
    // handle history
    if ((strcmp(newCmd[0], "history") == 0)) {
	for (int i = 0; i < histIndex; i++) {
	    printf(" %d %s\n", i, histCmd[i]);
	}
	return CONTINUE_SHELL;
    }

    // handle exit, quit command.
    if ((strcmp(newCmd[0], "exit") == 0) || (strcmp(newCmd[0], "quit") == 0)) {
	closeSh();
	exit(EXIT_SUCCESS);
    }

    // handle cd command.
    if (strcmp(newCmd[0], "cd") == 0) {
	if (newCmd[1] == NULL)
	    chdir(getenv("HOME"));
	else {
	    if (chdir(newCmd[1]) == -1)
		printf("-mysish: %s: %s: No such file or directory\n", newCmd[0], newCmd[1]);
	}
	// return CONTUNUE_SEHLL, which is 1.
	return CONTINUE_SHELL;
    }
    return 0;
}

/*
 * void readPath(char* newPath[], char path[]);
 *	This function stores environment variables in newPath[] array using getenv() function.
 *
 * argument: char*, char.
 *	*newPath[]  : Array that will store a singles PATH environment variable.
 *	path[]      : Array that stores whole PATH environ ment variables.
 * return:
 *	none.
 */
void readPath(char* newPath[], char path[]) {
    int innerLoopIndex = 1;
    char* tempPath;
    char* savePtr = NULL;

    // newPath[0]
    strcpy(path, getenv("PATH"));
    newPath[0] = strtok_r(path, ":", &savePtr);

    // newPath[1], newPath[2], ...
    while (1) {
	newPath[innerLoopIndex] = strtok_r(NULL, ":", &savePtr);
	if (newPath[innerLoopIndex] == NULL)
	    break;

	innerLoopIndex++;
    }
    return;
}

/*
 * void execProcess(char* newPath[], char* newCmd[]);
 *	This function excutes parent and child process.
 *
 * arguments:
 *	*newPath[]  :
 *	*newCmd[]   :
 * return:
 *	none.
 */
void execProcess(char* newPath[], char* newCmd[]) {
    extern char** environ;
    pid_t cpid;

    switch (cpid = fork()) {
	case -1:
	    perror("fork error.");
	    break;

	case 0:
	    // child process executes command.
	    execObj(newCmd, environ);
	    execCmd(newPath, newCmd, environ);

	    // in case of failure.
	    exit(EXIT_SUCCESS);
	    break;
	
	default:
	    // parent process waits.
	    wait(0);
	    break;
    }
    return;
}

/*
 * void execObj(char* newCmd[], char** environ);
 *	This function excutes user's compiled programs at child process.
 *
 * argument: char*, char**.
 *	*newCmd[]   : Set of command, option and argument.
 *	**environ   : Extern environ pointer.
 * return:
 *	none.
 */
void execObj(char* newCmd[], char** environ) {
    char curPath[MAX_SIZE];

    strcpy(curPath, newCmd[0]);
    if (execve(curPath, newCmd, environ) != -1)
	exit(EXIT_SUCCESS);
    return;
}

/*
 * void execBuiltIn(char* newPath[], char* newCmd[], char** environ);
 *	This function executes non- builtin commands at child process.
 *
 * argument: char*, char*, char**
 *	*newPath[]  : environ variable of PATH.
 *	*newCmd[]   : Set of command, option, and argument.
 *	**environ   : Extern environ pointer. 
 * return:
 *	none.
 */
void execCmd(char* newPath[], char* newCmd[], char** environ) {
    char curPath[MAX_SIZE];
    int innerLoopIndex = 0;

    do {
	if (newPath[innerLoopIndex] == NULL) {
	    printf("No command '%s' found.\n", newCmd[0]);
	    break;
	}
	else {
	    strcpy(curPath, newPath[innerLoopIndex]);
	    strcat(curPath, "/");
	    strcat(curPath, newCmd[0]);
	    strcat(curPath, "");
	}
        innerLoopIndex++;	
    } while (execve(curPath, newCmd, environ) == -1);

    exit(EXIT_SUCCESS);
    return;
}
