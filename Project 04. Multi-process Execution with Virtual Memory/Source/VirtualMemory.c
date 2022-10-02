/*
 * class		: Operating System(MS)
 * Project 02	: Multi-process Execution with Virtual Memory
 * Author		: Jaeil Park, Junseok Tak
 * Student ID	: 32161786, 32164809
 * Date			: 2021-12-06
 * Professor	: Seehwan Yoo
 * Left freedays: 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAX_PROCESS 10
#define TIME_TICK 10000// 0.01초.
#define TIME_QUANTUM 5// 0.05초.

typedef struct Node {
	struct Node* next;
	int procNum;
	int pid;
	int cpuTime;
	int ioTime;
} Node;

/* 스케줄링 정책에 사용되는 큐를 관리하는 리스트 구조체 */
typedef struct List {
	Node* head;
	Node* tail;
} List;

/* 페이지 테이블 엔트리 구조체 */
typedef struct Table {
	int* validBit;		// 1레벨 페이지 테이블 엔트리의 유효 비트
	int* tableNumber;	// 1레벨 페이지 테이블 엔트리의 테이블 번호

	int stateBit;		// 2레벨 테이블 페이지의 사용 여부를 구분하는 비트
	// int* validBit;	// 2레벨 페이지 테이블 엔트리의 유효 비트(위의 validBit를 사용)
	int* frameNumber;	// 2레벨 페이지 테이블 엔트리의 프레임 번호
	int* presentBit;	// 페이지 스왑 여부를 구분하는 비트 
} Table;

/* 메시지 큐를 통해 주고받을 CPU, IO 시간 구조체 */
struct DataOfIoCpu {
	int pid;
	int cpuTime;
	int ioTime;
};

struct msgBufIOCPU {
	long mtype;
	struct DataOfIoCpu mdata;
};

/* 메시지 큐를 통해 주고받을 논리 주소 구조체 */
struct DataOfMemAccess {
	int mLogicalAddress[10];
};

struct msgBufMemAccess {
	long mtype;
	struct DataOfMemAccess mdata;
};

/* 스케줄링 정책 중 큐와 노드를 관리하는 함수 */
void initList(List* list);
void pushBackNode(List* list, int procNum, int cpuTime, int ioTime);
void popFrontNode(List* list, Node* runNode);
bool isEmptyList(List* list);
void removeNode(List* list);
void writeNode(List* readyQueue, List* waitQueue, Node* cpuRunNode, FILE* wpburst);

/* 스케줄링 정책 중 신호와 데이터를 주고받는 시그널 함수와 메시지 큐 함수 */
void signalTimeTick(int signo);
void signalCPUSchedOut(int signo);
void signalIOSchedIn(int signo);
void cMsgSndIOCPU(int key, int cpuBurstTime, int ioBurstTime);
void pMsgRcvIOCPU(int procNum, Node* node);
void cMsgSndMemAccess(int procNum, int* cLogicalAddress);
void pMsgRcvMemAccess(int procNum, int* pLogicalAddress);

/* 2레벨 페이징과 스와핑이 적용된 가상 메모리를 관리하는 함수 */
int MMU(int* pLogicalAddress, int procNum);
int searchFreeLV2PageTable(Table* LV2Table);
int searchFreeFrame(int* frameList, int option);
int searchLRUPage(int* frameList);
void copyPage(int* dst, int dstNumber, int* dstList, int* src, int srcNumber, int* srcList);

/* 스케줄링 정책과 가상 메모리를 시뮬레이션하기 위해 메모리를 동적 할당하는 함수 */
void allocForSchedulingPolicy(void);
void allocForVirtualMemory(void);
void freeForSchedulingPolicy(void);
void freeForVirtualMemory(void);

List* waitQueue;
List* readyQueue;
List* subReadyQueue;
Node* cpuRunNode;
Node* ioRunNode;
FILE* rpburst;
FILE* wpburst;
FILE* wpmemory;
Table* LV1Table;
Table* LV2Table;

int CPID[MAX_PROCESS];
int KEY[MAX_PROCESS];
int CONST_TICK_COUNT;
int TICK_COUNT;
int RUN_TIME;
int* MEMORY;
int* DISK;
int* LRU;
int* memFrameList;
int memFrameListSize;
int* diskFrameList;

/* main 함수 */
int main(int argc, char* argv[]) {
	int originCpuBurstTime[3000];
	int originIoBurstTime[3000];
	int ppid = getpid();

	CONST_TICK_COUNT = 0;
	TICK_COUNT = 0;
	RUN_TIME = 0;

	allocForVirtualMemory();
	allocForSchedulingPolicy();

	struct itimerval new_itimer;
	struct itimerval old_itimer;

	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = TIME_TICK;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;

	struct sigaction tick;
	struct sigaction cpu;
	struct sigaction io;

	memset(&tick, 0, sizeof(tick));
	memset(&cpu, 0, sizeof(cpu));
	memset(&io, 0, sizeof(io));

	tick.sa_handler = &signalTimeTick;
	cpu.sa_handler = &signalCPUSchedOut;
	io.sa_handler = &signalIOSchedIn;

	sigaction(SIGALRM, &tick, NULL);
	sigaction(SIGUSR1, &cpu, NULL);
	sigaction(SIGUSR2, &io, NULL);

	// ScheduleDump.txt를 쓰기 모드로 엽니다.
	wpburst = fopen("ScheduleDump.txt", "w");
	if (wpburst == NULL) {
		perror("file open error");
		exit(EXIT_FAILURE);
	}
	fclose(wpburst);

	// MemoryDump.txt를 쓰기 모드로 엽니다.
	wpmemory = fopen("MemoryDump.txt", "w");
	if (wpmemory == NULL) {
		perror("file open error");
		exit(EXIT_FAILURE);
	}
	fclose(wpmemory);

	// 메시지 큐를 생성할 키 값을 저장합니다.
	for (int i = 0; i < MAX_PROCESS; i++) {
		// 논리 주소를 주고받을 메시지 큐의 키 값은 0x6123의 배수입니다.
		KEY[i] = 0x6123 * (i + 1);
		msgctl(msgget(KEY[i], IPC_CREAT | 0666), IPC_RMID, NULL);

		// 입출력 시간을 주고받을 메시지 큐의 키 값은 0x3216의 배수입니다.
		KEY[i] = 0x3216 * (i + 1);
		msgctl(msgget(KEY[i], IPC_CREAT | 0666), IPC_RMID, NULL);
	}

	// 메인 함수의 인자를 관리합니다.
	if (argc == 1 || argc == 2) {
		printf("COMMAND <TEXT FILE> <RUN TIME(sec)>\n");
		printf("./VirtualMemory.out TimeSet.txt 10\n");
		exit(EXIT_SUCCESS);
	}
	else {
		int preCpuTime;
		int preIoTime;

		// TimeSet.txt 파일을 엽니다.
		rpburst = fopen((char*)argv[1], "r");
		if (rpburst == NULL) {
			perror("file open error");
			exit(EXIT_FAILURE);
		}
		// TimeSet.txt 파일을 읽습니다.
		for (int innerLoopIndex = 0; innerLoopIndex < 3000; innerLoopIndex++) {
			if (fscanf(rpburst, "%d , %d", &preCpuTime, &preIoTime) == EOF) {
				printf("fscanf error");
				exit(EXIT_FAILURE);
			}
			originCpuBurstTime[innerLoopIndex] = preCpuTime;
			originIoBurstTime[innerLoopIndex] = preIoTime;
		}

		// 프로그램 실행 시간을 저장합니다.
		RUN_TIME = atoi(argv[2]) * 1000000 / TIME_TICK;
	}
	printf("TIME TICK   PROC NUMBER   REMAINED CPU TIME\n");

	/* 가상 메모리가 도입된 라운드 로빈 스케줄링 정책 */
	for (int j = 0; j < MAX_PROCESS; j++) {
		int ret = fork();

		// 부모 프로세스 부분.
		if (ret > 0) {
			CPID[j] = ret;
			pushBackNode(readyQueue, j, originCpuBurstTime[j], originIoBurstTime[j]);
		}

		// 자식 프로세스 부분.
		else {
			int BurstCycle = 1;
			int procNum = j;
			int cpuBurstTime = originCpuBurstTime[procNum];
			int ioBurstTime = originIoBurstTime[procNum];
			int cLogicalAddress[10];

			// 자식 프로세스는 타임 틱이 발생할 때까지 기다립니다.
			kill(getpid(), SIGSTOP);

			/* CPU 작업 부분 */
			while (true) {
				cpuBurstTime--;// CPU 시간을 1만큼 감소시킵니다.
				printf("            %02d            %02d\n", procNum, cpuBurstTime);
				printf("───────────────────────────────────────────\n");

				// 무작위 논리 주소 10개를 생성합니다.
				for (int i = 0; i < 10; i++) {
					int temp = rand();
					srand(time(NULL) + temp);
					cLogicalAddress[i] = rand() % 0x80000;// 프로세스 최대 사용 메모리: 512KB(512 * 2^10bytes)
				}

				// 생성한 논리 주소를 메시지 큐에 보냅니다.
				cMsgSndMemAccess(procNum, cLogicalAddress);

				// CPU 작업이 아직 남아있는 경우, SIGUSR1 신호를 보냅니다.
				if (cpuBurstTime != 0) {
					kill(ppid, SIGUSR1);
				}
				// CPU 작업이 끝난 경우, SIGUSR2 신호를 보냅니다.
				else {
					// CPU, IO 시간을 재설정합니다.
					cpuBurstTime = originCpuBurstTime[procNum + (BurstCycle * 10)];
					cMsgSndIOCPU(KEY[procNum], cpuBurstTime, ioBurstTime);
					ioBurstTime = originIoBurstTime[procNum + (BurstCycle * 10)];

					BurstCycle++;
					BurstCycle = BurstCycle > 298 ? 1 : BurstCycle;
					kill(ppid, SIGUSR2);
				}

				// 자식 프로세스는 다음 틱이 발생할 때까지 기다립니다.
				kill(getpid(), SIGSTOP);
			}
		}
	}

	// 준비 큐에서 첫 번째 노드를 실행 노드로 올립니다.
	popFrontNode(readyQueue, cpuRunNode);
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	// 실행 시간이 종료될 때까지 반복합니다.
	while (RUN_TIME != 0);

	// 메시지 큐를 삭제하고 자식 프로세스를 종료합니다.
	for (int i = 0; i < MAX_PROCESS; i++) {
		KEY[i] = 0x3216 * (i + 1);
		msgctl(msgget(KEY[i], IPC_CREAT | 0666), IPC_RMID, NULL);
	}
	for (int i = 0; i < MAX_PROCESS; i++) {
		KEY[i] = 0x6123 * (i + 1);
		msgctl(msgget(KEY[i], IPC_CREAT | 0666), IPC_RMID, NULL);
	}
	for (int i = 0; i < MAX_PROCESS; i++) {
		kill(CPID[i], SIGKILL);
	}
	writeNode(readyQueue, waitQueue, cpuRunNode, wpburst);

	// 동적 할당받은 메모리를 모두 해제합니다.
	freeForSchedulingPolicy();
	freeForVirtualMemory();
	return 0;
}


/*
* void signalTimeTick(int signo);
*	이 함수는 타임 틱이 발생할 때마다 호출하는 SIGALRM 신호의 핸들러 함수입니다.
*/
void signalTimeTick(int signo) { /* SIGALRM */
	if (RUN_TIME == 0)
		return;

	// ScheduleDump.txt 파일에 현재 상황을 기록합니다.
	writeNode(readyQueue, waitQueue, cpuRunNode, wpburst);

	// 틱 카운트를 1만큼 증가시킵니다.
	CONST_TICK_COUNT++;
	printf("%05d       PROC NUMBER   REMAINED CPU TIME\n", CONST_TICK_COUNT);

	/* IO 작업 부분 */
	Node* nodePtr = waitQueue->head;
	int waitQueueSize = 0;

	// 대기 큐의 크기를 구합니다.
	while (nodePtr != NULL) {
		nodePtr = nodePtr->next;
		waitQueueSize++;
	}

	for (int i = 0; i < waitQueueSize; i++) {
		popFrontNode(waitQueue, ioRunNode);
		ioRunNode->ioTime--;// IO 실행 시간을 1만큼 감소시킵니다.

		// IO 작업이 끝난 경우, 준비 큐로 노드를 보냅니다.
		if (ioRunNode->ioTime == 0)
			pushBackNode(readyQueue, ioRunNode->procNum, ioRunNode->cpuTime, ioRunNode->ioTime);

		// IO 작업이 남아있는 경우, 대기 큐로 노드를 보냅니다.
		else
			pushBackNode(waitQueue, ioRunNode->procNum, ioRunNode->cpuTime, ioRunNode->ioTime);
	}

	/* CPU 작업 부분 */
	if (cpuRunNode->procNum != -1) {
		kill(CPID[cpuRunNode->procNum], SIGCONT);
	}

	// 실행 시간을 1만큼 감소시킵니다.
	RUN_TIME--;
	return;
}

/*
* void signalCPUSchedOut(int signo);
*   이 함수는 현재 CPU를 선점한 프로세스를 준비 큐의 끝으로 보내고
*	다음 프로세스를 준비 큐에서 불러와 CPU를 선점할 수 있도록 합니다.
*/
void signalCPUSchedOut(int signo) { /* SIGUSR1 */
	int pLogicalAddress[10];
	TICK_COUNT++;

	/* 메모리 관리 부분 */
	pMsgRcvMemAccess(cpuRunNode->procNum, pLogicalAddress);
	MMU(pLogicalAddress, cpuRunNode->procNum);

	/* 스케줄링 정책 부분 */
	if (TICK_COUNT >= TIME_QUANTUM) {// 스케줄러는 타임 퀀텀마다 CPU를 선점할 프로세스를 변경합니다.
		pushBackNode(readyQueue, cpuRunNode->procNum, cpuRunNode->cpuTime, cpuRunNode->ioTime);
		popFrontNode(readyQueue, cpuRunNode);
		TICK_COUNT = 0;
	}
	return;
}

/*
* void signalIOSchedIn(int signo);
*	이 함수는 자식 프로세스가 입출력 작업이 있는지 확인합니다.
*	그리고 현재 CPU를 선점한 프로세스를 준비 큐 또는 대기 큐의 끝으로 보내고
*   다음 프로세스를 준비 큐에서 불러와 CPU를 선점할 수 있도록 합니다.
*/
void signalIOSchedIn(int signo) { /* SIGUSR2 */
	int pLogicalAddress[10];
	pMsgRcvIOCPU(cpuRunNode->procNum, cpuRunNode);

	/* 메모리 관리 부분 */
	pMsgRcvMemAccess(cpuRunNode->procNum, pLogicalAddress);
	MMU(pLogicalAddress, cpuRunNode->procNum);

	/* 스케줄링 정책 부분 */
	if (cpuRunNode->ioTime == 0)// 프로세스가 IO 작업이 없는 경우, 준비 큐의 끝으로 들어갑니다.
		pushBackNode(readyQueue, cpuRunNode->procNum, cpuRunNode->cpuTime, cpuRunNode->ioTime);

	else// 프로세스가 IO 작업이 없는 경우, 대기 큐의 끝으로 들어갑니다.
		pushBackNode(waitQueue, cpuRunNode->procNum, cpuRunNode->cpuTime, cpuRunNode->ioTime);

	// 준비 큐에서 다음으로 실행할 프로세스를 선택합니다.
	popFrontNode(readyQueue, cpuRunNode);
	TICK_COUNT = 0;
	return;
}

/*
* void initList(List* list);
*   이 함수는 리스트를 NULL로 초기화합니다.
*/
void initList(List* list) {
	list->head = NULL;
	list->tail = NULL;
	return;
}

/*
* void pushBackNode(List* list, int procNum, int cpuTime, int ioTime);
*	이 함수는 리스트에 새로운 노드를 생성하여 뒤쪽 노드로 넣는 함수입니다.
*/
void pushBackNode(List* list, int procNum, int cpuTime, int ioTime) {
	Node* newNode = (Node*)malloc(sizeof(Node));
	if (newNode == NULL) {
		perror("push node malloc error");
		exit(EXIT_FAILURE);
	}
	newNode->next = NULL;
	newNode->procNum = procNum;
	newNode->cpuTime = cpuTime;
	newNode->ioTime = ioTime;

	if (list->head == NULL) {
		list->head = newNode;
		list->tail = newNode;
	}
	else {
		list->tail->next = newNode;
		list->tail = newNode;
	}
	return;
}

/*
* void popFrontNode(List* list, Node* runNode);
*	이 함수는 리스트의 앞쪽 노드를 빼는 함수입니다.
*/
void popFrontNode(List* list, Node* runNode) {
	Node* NodePtr = list->head;

	// 리스트가 비어있는 경우.
	if (isEmptyList(list) == true) {
		runNode->cpuTime = -1;
		runNode->ioTime = -1;
		runNode->procNum = -1;
		return;
	}

	// 마지막 노드를 팝하는 경우.
	if (list->head->next == NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
	else
		list->head = list->head->next;

	*runNode = *NodePtr;
	free(NodePtr);
	return;
}

/*
* bool isEmptyList(List* list);
*   이 함수는 리스트가 비어있는지 확인합니다.
*/
bool isEmptyList(List* list) {
	if (list->head == NULL)
		return true;
	else
		return false;
}

/*
* void removeNode(List* list);
*	이 함수는 리스트에 연결된 노드를 해제합니다.
*/
void removeNode(List* list) {
	while (isEmptyList(list) == false) {
		Node* nodePtr = list->head;
		list->head = list->head->next;
		free(nodePtr);
	}
}

/*
* void cMsgSndIOCPU(int key, int cpuBurstTime, int ioBurstTime);
*	이 함수는 자식 프로세스가 CPU, IO 시간 데이터를 메시지 큐로 보내는 함수입니다.
*/
void cMsgSndIOCPU(int key, int cpuBurstTime, int ioBurstTime) {
	int qid = msgget(key, IPC_CREAT | 0666);// 메시지 큐 식별자를 생성합니다.

	struct msgBufIOCPU msg;
	memset(&msg, 0, sizeof(msg));

	msg.mtype = 1;
	msg.mdata.pid = getpid();
	msg.mdata.cpuTime = cpuBurstTime;
	msg.mdata.ioTime = ioBurstTime;

	// 부모 프로세스에게 데이터를 보냅니다.
	if (msgsnd(qid, (void*)&msg, sizeof(struct DataOfIoCpu), 0) == -1) {
		perror("child msgsnd error");
		exit(EXIT_FAILURE);
	}
	return;
}

/*
* void cMsgSndMemAccess(int procNum, int* cLogicalAddress);
*   이 함수는 자식 프로세스가 논리 주소 데이터를 메시지 큐로 보내는 함수입니다.
*/
void cMsgSndMemAccess(int procNum, int* cLogicalAddress) {
	int key = 0x6123 * (procNum + 1);
	int qid = msgget(key, IPC_CREAT | 0666);// 메시지 큐 식별자를 생성합니다.

	struct msgBufMemAccess msg;
	memset(&msg, 0, sizeof(msg));
	msg.mtype = 1;

	// 매개 변수의 논리 주소를 구조체에 저장합니다.
	for (int i = 0; i < 10; i++) {
		msg.mdata.mLogicalAddress[i] = cLogicalAddress[i];
	}

	// 부모 프로세스에게 데이터를 보냅니다.
	if (msgsnd(qid, (void*)&msg, sizeof(struct DataOfMemAccess), 0) == -1) {
		perror("child msgsnd error");
		exit(EXIT_FAILURE);
	}
	return;
}

/*
* void pMsgRcvIOCPU(int procNum, Node* node);
*   이 함수는 자식 프로세스가 보낸 CPU, IO 시간 데이터를 부모 프로세스가 받는 역할을 합니다.
*/
void pMsgRcvIOCPU(int procNum, Node* node) {
	int key = 0x3216 * (procNum + 1);// 메시지 큐 식별자를 생성합니다.
	int qid = msgget(key, IPC_CREAT | 0666);

	struct msgBufIOCPU msg;
	memset(&msg, 0, sizeof(msg));

	// 자식 프로세스가 보낸 데이터를 전달받습니다.
	if (msgrcv(qid, (void*)&msg, sizeof(msg), 0, 0) == -1) {
		perror("msgrcv error");
		exit(1);
	}
	node->pid = msg.mdata.pid;
	node->cpuTime = msg.mdata.cpuTime;
	node->ioTime = msg.mdata.ioTime;
	return;
}

/*
* void pMsgRcvMemAccess(int procNum, int* VAbuffer);
*	이 함수는 자식 프로세스가 보낸 논리 주소 데이터를 부모 프로세스가 받는 역할을 합니다.
*/
void pMsgRcvMemAccess(int procNum, int* pLogicalAddress) {
	int key = 0x6123 * (procNum + 1);// 메시지 큐 식별자를 생성합니다.
	int qid = msgget(key, IPC_CREAT | 0666);

	struct msgBufMemAccess msg;
	memset(&msg, 0, sizeof(msg));

	// 자식 프로세스가 보낸 데이터를 전달받습니다.
	if (msgrcv(qid, (void*)&msg, sizeof(msg), 0, 0) == -1) {
		perror("msgrcv error");
		exit(1);
	}
	for (int i = 0; i < 10; i++) {
		pLogicalAddress[i] = msg.mdata.mLogicalAddress[i];
	}
	return;
}

/*
* void writeNode(List* readyQueue, List* waitQueue, Node* cpuRunNode, FILE* wpburst);
*   이 함수는 준비 큐, 대기 큐 상태를 ScheduleDump.txt 파일에 출력합니다.
*/
void writeNode(List* readyQueue, List* waitQueue, Node* cpuRunNode, FILE* wpburst) {
	Node* nodePtr1 = readyQueue->head;
	Node* nodePtr2 = waitQueue->head;

	wpburst = fopen("ScheduleDump.txt", "a+");// ScheduleDump.txt 파일을 append 모드로 엽니다.
	fprintf(wpburst, "───────────────────────────────────────────────────────\n");
	fprintf(wpburst, " TICK   %04d\n\n", CONST_TICK_COUNT);
	fprintf(wpburst, " RUNNING PROCESS\n");
	fprintf(wpburst, " %02d\n\n", cpuRunNode->procNum);
	fprintf(wpburst, " READY QUEUE\n");

	if (nodePtr1 == NULL)
		fprintf(wpburst, " none");
	while (nodePtr1 != NULL) {// 준비 큐의 상태를 출력합니다.
		fprintf(wpburst, " %02d ", nodePtr1->procNum);
		nodePtr1 = nodePtr1->next;
	}

	fprintf(wpburst, "\n\n");
	fprintf(wpburst, " WAIT QUEUE\n");

	if (nodePtr2 == NULL)
		fprintf(wpburst, " none");
	while (nodePtr2 != NULL) {// 대기 큐의 상태를 출력합니다.
		fprintf(wpburst, " %02d ", nodePtr2->procNum);
		nodePtr2 = nodePtr2->next;
	}

	fprintf(wpburst, "\n");
	fclose(wpburst);
	return;
}

/*
* int MMU(int* pLogicalAddress, int procNum);
*	이 함수는 다음과 같은 단계로 주소를 변환합니다.
*	Step 0. 메모리 상황에 따라서 스왑 인, 스왑 아웃합니다.
*	Step 1. 논리 주소를 세 부분으로 나눕니다.
*	Step 2. 1레벨 페이지 히트, 폴트를 확인합니다.
*	Step 3. 2레벨 페이지 히트, 폴트를 확인합니다.
*	Step 4. 물리 주소로 메모리에 데이터를 읽고 씁니다.
*/
int MMU(int* pLogicalAddress, int procNum) {
	int logicalAddress;
	int LV1PageNumber, LV2PageNumber, offset;
	int LV2TableNumber, frameNumber;
	int data, LRUpage, procNumber;
	int diskAddress;

	wpmemory = fopen("MemoryDump.txt", "a+");
	fprintf(wpmemory, "───────────────────────────────────────────\n");
	fprintf(wpmemory, " TICK   %04d\n", CONST_TICK_COUNT);
	fprintf(wpmemory, "───────────────────────────────────────────\n");

	for (int i = 0; i < 10; i++) {
		logicalAddress = pLogicalAddress[i];
		fprintf(wpmemory, " %d번째 논리 주소 0x%x\n", i, logicalAddress);

		/* Step 0. 스왑 아웃 */
		if (memFrameListSize < 0x100) {// 메모리 3840(4096 - 256) 이상을 사용할 경우, 스왑 아웃이 발생합니다.
			fprintf(wpmemory, " 스왑 아웃 [o] ");
			LRUpage = searchLRUPage(memFrameList);// 가장 적게 사용한 페이지를 찾습니다.
			LV1PageNumber = (memFrameList[LRUpage] >> 26) & 0x3F;
			LV2PageNumber = (memFrameList[LRUpage] >> 20) & 0x3F;
			procNumber = (memFrameList[LRUpage] >> 16) & 0xF;

			LV2TableNumber = LV1Table[procNumber].tableNumber[LV1PageNumber];
			LV2Table[LV2TableNumber].presentBit[LV2PageNumber] = 1;

			diskAddress = searchFreeFrame(diskFrameList, 1);// 디스크에서 비어있는 공간을 찾습니다.
			LV2Table[LV2TableNumber].frameNumber[LV2PageNumber] = diskAddress;
			copyPage(DISK, diskAddress, diskFrameList, MEMORY, LRUpage, memFrameList);
			memFrameListSize++;
			fprintf(wpmemory, "(메모리[0x%x - 0x%x] ▶ 디스크[0x%x - 0x%x])\n", LRUpage * 0x400, ((LRUpage + 1) * 0x400) - 1, diskAddress * 0x400, ((diskAddress + 1) * 0x400) - 1);
		}
		else
			fprintf(wpmemory, " 스왑 아웃 [x]\n");

		/* Step 1: 논리 주소를 1레벨 페이지 번호(6비트), 2레벨 페이지 번호(6비트), 그리고 변위(10비트)로 구분합니다. */
		LV1PageNumber = (logicalAddress >> 16) & 0x3F;
		LV2PageNumber = (logicalAddress >> 10) & 0x3F;
		offset = logicalAddress & 0x3FF;

		/* Step 2: 1레벨 페이지 폴트 */
		if (LV1Table[procNum].validBit[LV1PageNumber] == 0) {
			fprintf(wpmemory, " 1레벨 페이지 폴트 [o]\n");
			fprintf(wpmemory, " 1레벨 페이지 히트 [x]\n");

			// 비어있는 2레벨 페이지 테이블을 찾아서 연결합니다.
			LV2TableNumber = searchFreeLV2PageTable(LV2Table);
			LV1Table[procNum].tableNumber[LV1PageNumber] = LV2TableNumber;
			LV1Table[procNum].validBit[LV1PageNumber] = 1;
		}

		/* Step 2: 1레벨 페이지 히트 */
		else {
			fprintf(wpmemory, " 1레벨 페이지 폴트 [x]\n");
			fprintf(wpmemory, " 1레벨 페이지 히트 [o]\n");

			// 1레벨 테이블 페이지에서 테이블 번호를 불러옵니다.
			LV2TableNumber = LV1Table[procNum].tableNumber[LV1PageNumber];
		}

		/* Step 3: 2레벨 페이지 폴트 */
		if (LV2Table[LV2TableNumber].validBit[LV2PageNumber] == 0) {
			fprintf(wpmemory, " 2레벨 페이지 폴트 [o]\n");
			fprintf(wpmemory, " 2레벨 페이지 히트 [x]\n");
			fprintf(wpmemory, " 스왑 인 [x]\n");

			// 비어있는 페이지 프레임을 찾아서 연결합니다.
			frameNumber = searchFreeFrame(memFrameList, 0);
			LV2Table[LV2TableNumber].frameNumber[LV2PageNumber] = frameNumber;
			LV2Table[LV2TableNumber].validBit[LV2PageNumber] = 1;

			// 메모리 프리 프레임 리스트에 1레벨 페이지 테이블 번호, 2레벨 페이지 테이블 번호, 프로세스 번호를 저장합니다.
			// 프리 프레임 리스트 항목은 LV1 page number(6bits), LV2 page number(6bits), process number(4bits), empty bits(15bits), valid bit(1bit)로 구성되어 있습니다.	
			memFrameList[frameNumber] += ((LV1PageNumber & 0x3F) << 26);
			memFrameList[frameNumber] += ((LV2PageNumber & 0x3F) << 20);
			memFrameList[frameNumber] += ((procNum & 0xF) << 16);
		}
		/* Step 3: 2레벨 페이지 히트 */
		else {
			fprintf(wpmemory, " 2레벨 페이지 폴트 [x]\n");
			fprintf(wpmemory, " 2레벨 페이지 히트 [o]\n");

			/* Step 0: 스왑 인 */
			if (LV2Table[LV2TableNumber].presentBit[LV2PageNumber] == 1) {// 스왑 아웃된 페이지의 경우, 스왑 인을 합니다.
				fprintf(wpmemory, " 스왑 인 [o] ");
				frameNumber = searchFreeFrame(memFrameList, 0);
				diskAddress = LV2Table[LV2TableNumber].frameNumber[LV2PageNumber];// 디스크에 저장된 페이지를 불러옵니다.
				copyPage(MEMORY, frameNumber, memFrameList, DISK, diskAddress, diskFrameList);
				fprintf(wpmemory, "(디스크[0x%x - 0x%x] ▶ 메모리[0x%x - 0x%x])\n", diskAddress * 0x400, ((diskAddress + 1) * 0x400) - 1, frameNumber * 0x400, ((frameNumber + 1) * 0x400) - 1);

				LV2Table[LV2TableNumber].presentBit[LV2PageNumber] = 0;
				LV2Table[LV2TableNumber].frameNumber[LV2PageNumber] = frameNumber;

				// 프리 프레임 리스트는 LV1 page number(6bits), LV2 page number(6bits), process number(4bits), empty bits(15bits), state bit(1bit)로 구성되어 있습니다.	
				memFrameList[frameNumber] += ((LV1PageNumber & 0x3F) << 26);
				memFrameList[frameNumber] += ((LV2PageNumber & 0x3F) << 20);
				memFrameList[frameNumber] += ((procNum & 0xF) << 16);
			}
			else {
				fprintf(wpmemory, " 스왑 인 [x]\n");
				// 2레벨 페이지에서 프레임 번호를 불러옵니다.
				frameNumber = LV2Table[LV2TableNumber].frameNumber[LV2PageNumber];
			}
			// LRU를 업데이트합니다.
			LRU[frameNumber]++;
		}
		fprintf(wpmemory, " %d번째 물리주소 0x%x ", i, ((frameNumber) * 0x400) + offset - (offset % 4));

		/* Step 4: 주소 변환과 메모리 접근 */
		// 프레임 번호와 변위로 물리 주소를 구합니다. 물리주소는 data(31bits), modified bit(1bit)로 구성되어 있습니다.
		data = MEMORY[((frameNumber * 0x400) + offset) / 4];

		if (((data >> 31) & 0x1) == 0) {
			// 이전에 데이터가 변경된 이력이 없을 경우, 메모리에 틱 카운트 데이터를 씁니다.
			MEMORY[((frameNumber * 0x400) + offset) / 4] = (CONST_TICK_COUNT + 0x80000000);// 0x80000000는 modified bit를 나타냅니다.
			fprintf(wpmemory, "(데이터 쓰기 %d)\n\n", CONST_TICK_COUNT);
		}
		else
			// 이전에 데이터가 변경된 이력이 있을 경우, 메모리의 틱 카운트 데이터를 읽습니다.
			fprintf(wpmemory, "(데이터 읽기 %d)\n\n", data - 0x80000000);
	}
	fclose(wpmemory);
	return 0;
}

/*
* int searchFreeLV2PageTable(Table* LV2Table);
*	이 함수는 1레벨 페이지 테이블에 할당되지 않은 2레벨 페이지 테이블을 찾습니다.
*/
int searchFreeLV2PageTable(Table* LV2Table) {
	int LV2PageTableNumber = 0;

	for (int i = 0; i < 0x280; i++) {// 2레벨 페이지 테이블 수: 640(0x280)
		if (LV2Table[LV2PageTableNumber].stateBit == 0) {
			LV2Table[LV2PageTableNumber].stateBit = 1;
			break;
		}
		LV2PageTableNumber++;
	}
	return LV2PageTableNumber;
}

/*
* int searchFreeFrame(int* List, int option);
*	이 함수는 프리 프레임 리스트에서 아직 사용되지 않은 비어있는 프레임을 찾습니다.
*/
int searchFreeFrame(int* frameList, int option) {
	int frameNumber = 0;

	// 프리 프레임 리스트 항목은 LV1 page number(6bits), LV2 page number(6bits), process number(4bits), empty bits(15bits), state bit(1bit)로 구성되어 있습니다.
	// 상태 비트(state bit)로 프레임 사용 여부를 구분합니다.
	for (int i = 0; i < 0x1000; i++) {
		if ((frameList[i] & 0x1) == 0) {
			if (option == 0)// 메모리에서 비어있는 프레임을 찾을 경우, 프리 프레임 리스트의 크기를 줄입니다.
				memFrameListSize--;

			frameNumber = i;
			frameList[i] = 1;// 상태 비트를 업데이트합니다.
			break;
		}
	}
	return frameNumber;
}

/*
* int searchLRUPage(int* list);
*	이 함수는 LRU 알고리즘에 따라서 가장 최근에 사용되지 않은 페이지를 선택합니다.
*/
int searchLRUPage(int* frameList) {
	int LRUPage = 0;
	int LRUCount = 999999999;

	// 프리 프레임 리스트는 LV1 page number(6bits), LV2 page number(6bits), process number(4bits), empty bits(15bits), state bit(1bit)로 구성되어 있습니다.	
	for (int i = 0; i < 0x1000; i++) {
		if ((frameList[i] & 0x1) == 1) {
			if (LRU[i] < LRUCount) {
				LRUPage = i;
				LRUCount = LRU[i];
				LRU[i]++;
			}
		}
	}
	return LRUPage;
}

/*
* void copyPage(int* dst, int dstNumber, int* dstList, int* src, int srcNumber, int* srcList);
*	이 함수는 스와핑에서 페이지와 디스크에 저장된 페이지를 맞바꾸고 비어있는 상태로 초기화합니다.
*/
void copyPage(int* dst, int dstNumber, int* dstList, int* src, int srcNumber, int* srcList) {
	for (int i = 0; i < 0x100; i++) {// 256(1KB / 4byte = 2^10 / 2^2 = 0x100)
		dst[(dstNumber * 0x100) + i] = src[(srcNumber * 0x100) + i];
		src[(srcNumber * 0x100) + i] = 0;// 페이지를 반대편 하드웨어로 복사하고 0으로 초기화합니다.
	}
	dstList[dstNumber] = 1;
	srcList[srcNumber] = 0;
}

/*
* void allocForSchedulingPolicy(void);
*	이 함수는 라운드 로빈 스케줄링 정책을 구현하는데 필요한 여러 가지 큐와 노드를 동적 할당합니다.
*/
void allocForSchedulingPolicy(void) {
	// 준비 큐, 보조 준비 큐, 대기 큐를 동적 할당합니다.
	waitQueue = malloc(sizeof(List));
	readyQueue = malloc(sizeof(List));
	subReadyQueue = malloc(sizeof(List));
	cpuRunNode = malloc(sizeof(Node));
	ioRunNode = malloc(sizeof(Node));

	if (waitQueue == NULL || readyQueue == NULL || subReadyQueue == NULL) {
		perror("list malloc error");
		exit(EXIT_FAILURE);
	}
	if (cpuRunNode == NULL || ioRunNode == NULL) {
		perror("node malloc error");
		exit(EXIT_FAILURE);
	}

	// 준비 큐, 보조 준비 큐, 대기 큐를 초기화합니다.
	initList(waitQueue);
	initList(readyQueue);
	initList(subReadyQueue);
}

/*
* void allocForSchedulingPolicy(void);
*	이 함수는 가상 메모리를 구현하는데 필요한 메모리, 디스크 등을 동적 할당합니다.
*
*	메모리 크기: 4MB
*	디스크 크기: 4MB
*	페이지 크기: 1KB
*	논리 주소를 1레벨 페이지 번호(6비트), 2레벨 페이지 번호(6비트), 변위(10비트)
*	메모리 속 프리 프레임 리스트 엔트리 개수: 4K(4MB / 1KB)
*	디스크 속 프리 프레임 리스트 엔트리 개수(스와핑): 4K
*/
void allocForVirtualMemory(void) {
	MEMORY = (int*)malloc(sizeof(int) * 0x100000);		// 메모리: 4MB(2^2 * 2^20 = 4bytes * 0x10:0000)
	DISK = (int*)malloc(sizeof(int) * 0x100000);		// 디스크: 4MB(2^2 * 2^20 = 4bytes * 0x10:0000)
	LRU = (int*)malloc(sizeof(int) * 0x1000);
	memFrameList = (int*)malloc(sizeof(int) * 0x1000);	// 메모리 프레임 리스트: 4K(2^12) 배열
	diskFrameList = (int*)malloc(sizeof(int) * 0x1000);	// 디스크 프레임 리스트: 4K(2^12) 배열
	memFrameListSize = 0x1000;							// 프레임 리스트 크기: 4096 숫자

	// 동적 할당한 메모리를 초기화합니다.
	memset(memFrameList, 0, malloc_usable_size(memFrameList));
	memset(diskFrameList, 0, malloc_usable_size(diskFrameList));
	memset(MEMORY, 0, malloc_usable_size(MEMORY));
	memset(DISK, 0, malloc_usable_size(DISK));
	memset(LRU, 0, malloc_usable_size(LRU));

	// 1레벨 페이지 테이블: (테이블 크기) * (사용자 프로세스) = sizeof(Table) * 10
	LV1Table = (Table*)malloc(sizeof(Table) * 0xA);
	
	// 2레벨 페이지 테이블: (테이블 크기) * (사용자 프로세스) * (1레벨 페이지 테이블 엔트리 수) = sizeof(Table) * 10 * 64
	LV2Table = (Table*)malloc(sizeof(Table) * 0xA * 0x40);

	// 1레벨 페이지 테이블 동적 할당(6bits)
	for (int i = 0; i < 10; i++) {
		LV1Table[i].tableNumber = malloc(sizeof(int) * 0x40);
		LV1Table[i].validBit = malloc(sizeof(int) * 0x40);
		for (int t = 0; t < 0x40; t++) {
			LV1Table[i].tableNumber[t] = 0;
			LV1Table[i].validBit[t] = 0;
		}
	}

	// 2레벨 페이지 테이블 동적 할당(6bits)
	for (int i = 0; i < 0x280; i++) {
		LV2Table[i].frameNumber = malloc(sizeof(int) * 0x40);
		LV2Table[i].presentBit = malloc(sizeof(int) * 0x40);
		LV2Table[i].validBit = malloc(sizeof(int) * 0x40);
		LV2Table[i].stateBit = 0;
		for (int t = 0; t < 0x40; t++) {
			LV2Table[i].frameNumber[t] = 0;
			LV2Table[i].validBit[t] = 0;
			LV2Table[i].presentBit[t] = 0;
		}
	}
}

/*
* void freeForSchedulingPolicy(void);
*	이 함수는 스케줄링 정책을 구현하는데 사용했던 동적 할당된 메모리를 해제합니다.
*/
void freeForSchedulingPolicy(void) {
	// 준비 큐, 보조 준비 큐, 대기 큐에 남아있는 노드를 삭제합니다.
	removeNode(readyQueue);
	removeNode(subReadyQueue);
	removeNode(waitQueue);

	// 준비 큐, 보조 준비 큐, 대기 큐를 해제합니다.
	free(readyQueue);
	free(subReadyQueue);
	free(waitQueue);
	free(cpuRunNode);
	free(ioRunNode);
}

/*
* void freeForVirtualMemory(void);
*	이 함수는 가상 메모리를 구현하는데 사용했던 동적 할당된 메모리를 해제합니다.
*/
void freeForVirtualMemory(void) {
	// 1레벨 페이지 테이블을 해제합니다.
	for (int i = 0; i < 10; i++) {
		free(LV1Table[i].tableNumber);
		free(LV1Table[i].validBit);
	}
	// 2레벨 페이지 테이블을 해제합니다.
	for (int i = 0; i < 0x280; i++) {
		free(LV2Table[i].frameNumber);
		free(LV2Table[i].presentBit);
		free(LV2Table[i].validBit);
	}
	free(LV1Table);
	free(LV2Table);
	free(MEMORY);
	free(DISK);
	free(LRU);
	free(memFrameList);
	free(diskFrameList);
}
