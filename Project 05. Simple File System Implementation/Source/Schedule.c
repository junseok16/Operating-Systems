#include "FileSystem.h"

/*
*	void signalTimeTick(int signo)
*	This is a signal handler function that is executed every tick by the SIGALRM signal.
*	If there is a waiting process in the waitQueue, the IO time is reduced by 1 tick and 
*	the file access operation is performed.
*/
void signalTimeTick(int signo) {								//SIGALRM
	char userBuffer[1024];
	int openMode;
	int procNum;

	if (RUN_TIME == 0) {
		return;
	}

	CONST_TICK_COUNT++;
	printf("TICK %05d\n", CONST_TICK_COUNT);

	// io burst part.
	PCB* PCBPtr = waitQueue->head;
	int waitQueueSize = 0;

	// get the size of wait queue.
	while (PCBPtr != NULL) {
		PCBPtr = PCBPtr->next;
		waitQueueSize++;
	}
	if (waitQueueSize == 0) {
		printf("No File Request\n");
	}

	for (int i = 0; i < waitQueueSize; i++) {
		popPCB(waitQueue, ioRunPCB);
		ioRunPCB->ioTime--;													// decrease io time by 1.
		procNum = ioRunPCB->procNum;
		printf("Proc[%02d] : ", procNum);

		if (ioRunPCB->openFile.fileCond == 0) {								//File not opened
			openMode = rand();
			srand(time(NULL) + openMode);
			openMode = (rand() % 2) + 1;									//openMode 1 -> read, openMode 2 -> write
			randFileSelect(ioRunPCB->openFile.fileName);					//Random File select
			if (fileOpen(ioRunPCB->openFile.fileName, openMode) == 0) {		//file oepn success
				ioRunPCB->openFile.fileCond = openMode;						//PCB File Condition Update
			}
			else {															//file open fail

			}
		}
		else if (ioRunPCB->openFile.fileCond == 1) {						//File already opened read mode
			if (fileRead(ioRunPCB, userBuffer, 11) < 10) {					//Read All File
				printf("File Read [%s]\n", userBuffer);
				printf("           ");
				ioRunPCB->openFile.offSet = 0;								//PCB offSet reset
				fileClose(ioRunPCB->openFile.fileName, ioRunPCB->openFile.fileCond);
				ioRunPCB->openFile.fileCond = 0;							//PCB File Condition reset
			}
			else {
				printf("File Read [%s]\n", userBuffer);
			}
		}
		else if (ioRunPCB->openFile.fileCond == 2) {						//File  already opend write mode
			fileWrite(ioRunPCB, "filechange");
			printf("           ");
			fileClose(ioRunPCB->openFile.fileName, ioRunPCB->openFile.fileCond);
			ioRunPCB->openFile.fileCond = 0;								//PCB File Condition reset
		}

		// io task is over, then push node to ready queue.
		if (ioRunPCB->ioTime == 0) {
			pushPCB(readyQueue, ioRunPCB->procNum, ioRunPCB->cpuTime, ioRunPCB->ioTime, ioRunPCB->openFile);
		}
		// io task is not over, then push node to wait queue again.
		else {
			pushPCB(waitQueue, ioRunPCB->procNum, ioRunPCB->cpuTime, ioRunPCB->ioTime, ioRunPCB->openFile);
		}
	}
	printf("--------------------------------------------------------------\n");
	// cpu burst part.
	if (cpuRunPCB->procNum != -1) {
		kill(CPID[cpuRunPCB->procNum], SIGCONT);
	}

	RUN_TIME--;// run time decreased by 1.
	return;
}

/*
*	void signalRRcpuSchedOut(int signo)
*	This is a signal handler function called by the SIGUSR1 signal.
*	Checks whether the process has worked as much as quantum ticks.
*/
void signalRRcpuSchedOut(int signo) {							//SIGUSR1
	TICK_COUNT++;

	// scheduler changes cpu preemptive process at every time quantum.
	if (TICK_COUNT >= TIME_QUANTUM) {
		pushPCB(readyQueue, cpuRunPCB->procNum, cpuRunPCB->cpuTime, cpuRunPCB->ioTime, cpuRunPCB->openFile);

		// pop the next process from the ready queue.
		popPCB(readyQueue, cpuRunPCB);
		TICK_COUNT = 0;
	}
	return;
}

/*
*	void signalIoSchedIn(int signo)
*	This is a signal handler function called by the SIGUSR2 signal.
*	When all CPU time is consumed, the process is moved to the waitQueue.
*/
void signalIoSchedIn(int signo) {								//SIGUSR2
	pMsgRcvIocpu(cpuRunPCB->procNum, cpuRunPCB);

	// process that has no io task go to the end of the ready queue.
	if (cpuRunPCB->ioTime == 0) {
		pushPCB(readyQueue, cpuRunPCB->procNum, cpuRunPCB->cpuTime, cpuRunPCB->ioTime, cpuRunPCB->openFile);
	}
	// process that has io task go to the end of the wait queue.
	else {
		pushPCB(waitQueue, cpuRunPCB->procNum, cpuRunPCB->cpuTime, cpuRunPCB->ioTime, cpuRunPCB->openFile);
	}

	// pop the next process from the ready queue.
	popPCB(readyQueue, cpuRunPCB);
	TICK_COUNT = 0;
	return;
}

/*
*	void initPCBList(PCBList* list)
*	A function that initializes a list.
*/
void initPCBList(PCBList* list) {
	list->head = NULL;
	list->tail = NULL;
	list->listSize = 0;
	return;
}

/*
*	void pushPCB(PCBList* list, int procNum, int cpuTime, int ioTime, PCBfile file)
*	A function that adds a node based on the parameters input to the list.
*/
void pushPCB(PCBList* list, int procNum, int cpuTime, int ioTime, PCBfile file) {
	PCB* newPCB = (PCB*)malloc(sizeof(PCB));
	if (newPCB == NULL) {
		perror("push PCB malloc error");
		exit(EXIT_FAILURE);
	}

	newPCB->next = NULL;
	newPCB->procNum = procNum;
	newPCB->cpuTime = cpuTime;
	newPCB->ioTime = ioTime;
	newPCB->openFile.fileCond = file.fileCond;
	newPCB->openFile.offSet = file.offSet;
	strcpy(newPCB->openFile.fileName, file.fileName);
	
	// the first node case.
	if (list->head == NULL) {
		list->head = newPCB;
		list->tail = newPCB;
	}
	// another node cases.
	else {
		list->tail->next = newPCB;
		list->tail = newPCB;
	}
	return;
}

/*
*	void popPCB(PCBList* list, PCB* runPCB)
*	A function that returns a node in a list.
*/
void popPCB(PCBList* list, PCB* runPCB) {
	PCB* oldPCB = list->head;

	// empty list case.
	if (isEmptyList(list) == true) {
		runPCB->cpuTime = -1;
		runPCB->ioTime = -1;
		runPCB->procNum = -1;
		return;
	}

	// pop the last PCB from a list case.
	if (list->head->next == NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
	else {
		list->head = list->head->next;
	}

	*runPCB = *oldPCB;
	free(oldPCB);
	return;
}

/*
*	bool isEmptyList(PCBList* list)
*	A function that checks whether the list is empty and returns a bool type.
*/
bool isEmptyList(PCBList* list) {
	if (list->head == NULL)
		return true;
	else
		return false;
}

/*
*	void deletePCB(PCBList* list)
*	A function that removes all nodes in a list.
*/
void deletePCB(PCBList* list) {
	while (isEmptyList(list) == false) {
		PCB* delPCB;
		delPCB = list->head;
		list->head = list->head->next;
		free(delPCB);
	}
}

/*
*	void cMsgSndIocpu(int procNum, int cpuBurstTime, int ioBurstTime)
*	A function that sends a message queue from a child process to a parent process.
*/
void cMsgSndIocpu(int procNum, int cpuBurstTime, int ioBurstTime) {
	int key = 0x3216 * (procNum + 1);
	int qid = msgget(key, IPC_CREAT | 0666);				// create message queue ID.

	struct msgBufIocpu msg;
	memset(&msg, 0, sizeof(msg));

	msg.mType = 1;
	msg.mData.pid = getpid();
	msg.mData.cpuTime = cpuBurstTime;						// child process cpu burst time.
	msg.mData.ioTime = ioBurstTime;							// child process io burst time.

	// child process sends its data to parent process.
	if (msgsnd(qid, (void*)&msg, sizeof(dataIocpu), 0) == -1) {
		perror("Child Msgsnd Error");
		exit(EXIT_FAILURE);
	}
	return;
}

/*
*	void pMsgRcvIocpu(int procNum, PCB* PCBPtr)
*	A function in which the parent process receives a message queue from the child process.
*/
void pMsgRcvIocpu(int procNum, PCB* PCBPtr) {
	int key = 0x3216 * (procNum + 1);						// create message queue key.
	int qid = msgget(key, IPC_CREAT | 0666);

	struct msgBufIocpu msg;
	memset(&msg, 0, sizeof(msg));

	// parent process receives child process data.
	if (msgrcv(qid, (void*)&msg, sizeof(msg), 0, 0) == -1) {
		perror("Msgrcv Error");
		exit(1);
	}

	// copy the data of child process to nodePtr.
	PCBPtr->pid = msg.mData.pid;
	PCBPtr->cpuTime = msg.mData.cpuTime;
	PCBPtr->ioTime = msg.mData.ioTime;
	return;
}
