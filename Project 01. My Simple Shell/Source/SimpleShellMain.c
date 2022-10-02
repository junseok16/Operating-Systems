/*
 * class	: Operating System(MS)
 * Homework 01	: My Simple Shell(mysish)
 * Author	: Tak junseok
 * Student ID	: 32164809
 * Date		: 2021-09-30
 * Professor	: Yoo seehwan
 * Left freedays: 5
 */

#include "SimpleShellHeader.h"

int main(void) {
    char cmd[MAX_SIZE];
    char path[MAX_SIZE];
    char* newCmd[MAX_SIZE];
    char* newPath[MAX_SIZE];
    int checkBit;
    histIndex = 0;
    
    openSh();
    while (1) {
	memset(cmd, 0, sizeof(cmd));
	memset(path, 0, sizeof(path));
	memset(newCmd, '\0', sizeof(newCmd));
	memset(newPath, '\0', sizeof(newPath));
	checkBit = 0;

	printPrompt();
	checkBit = readCmd(cmd, MAX_SIZE);
	if (checkBit == CONTINUE_SHELL) continue;

	parseCmd(newCmd, cmd);
	readPath(newPath, path);
	checkBit = execBuiltIn(newCmd);
	if (checkBit == CONTINUE_SHELL) continue;

	execProcess(newPath, newCmd);
    }
    return 0;
}
