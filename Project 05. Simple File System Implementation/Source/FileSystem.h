#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

//RR Define
#define MAX_PROCESS 10
#define TIME_TICK 10000						// 0.01 second(10ms).
#define TIME_QUANTUM 5						// 0.05 seconds(50ms).

//File System Define
#define SIMPLE_PARTITION	0x1111

#define INVALID_INODE			0
#define INODE_MODE_AC_ALL		0x777
#define INODE_MODE_AC_USER_R	0x001
#define INODE_MODE_AC_USER_W	0x002
#define INODE_MODE_AC_USER_X	0x004
#define INODE_MODE_AC_OTHER_R	0x010
#define INODE_MODE_AC_OTHER_W	0x020
#define INODE_MODE_AC_OTHER_X	0x040
#define INODE_MODE_AC_GRP_R		0x100
#define INODE_MODE_AC_GRP_W		0x200
#define INODE_MODE_AC_GRP_X		0x400

#define INODE_MODE_REG_FILE		0x10000
#define INODE_MODE_DIR_FILE		0x20000
#define INODE_MODE_DEV_FILE		0x40000

#define DENTRY_TYPE_REG_FILE	0x1
#define DENTRY_TYPE_DIR_FILE	0x2

#define BLOCK_SIZE				0x400
/*
  Partition structure
	ASSUME: data block size: 1K

	partition: 4MB

	Superblock: 1KB
	Inode table (32 bytes inode array) * 224 entries = 7KB
	data blocks: 1KB blocks array (~4K)
*/

//Super block structure
typedef struct superBlock {
	unsigned int partitionType;
	unsigned int blockSize;
	unsigned int inodeSize;
	unsigned int firstInode;

	unsigned int numInodes;
	unsigned int numInodeBlocks;
	unsigned int numFreeInodes;

	unsigned int numBlocks;
	unsigned int numFreeBlocks;
	unsigned int firstDataBlock;
	char volumeName[24];
	unsigned char padding[960]; //1024-64
} superBlock;


//32 byte I-node structure
typedef struct inode {
	unsigned int mode; 					// reg. file, directory, dev., permissions
	unsigned int locked; 				// opened for write
	unsigned int date;
	unsigned int size;
	int indirectBlock; 					// N.B. -1 for NULL
	unsigned short blocks[0x6];
} inode;

typedef struct blocks {
	unsigned char d[1024];
} blocks;

//physical partition structure
typedef struct partition {
	struct superBlock super;
	struct inode inodeTable[224];
	struct blocks dataBlocks[4088];		//4096-8
} partition;

//Directory entry structure
typedef struct dentry {
	unsigned int inode;
	unsigned int dirLength;
	unsigned int nameLen;
	unsigned int fileType;
	union { // name
		unsigned char name[256];
		unsigned char nPad[16][16];
	};
} dentry;

/*
	Round Robin Scheduling Struct
*/

typedef struct PCBfile {
	char fileName[20];
	int fileCond;						// 0->not open file, 1->open read, 2->open write
	int offSet;
} PCBfile;

typedef struct processControlBlock {
	struct processControlBlock* next;
	int procNum;
	int pid;
	int cpuTime;
	int ioTime;
	PCBfile openFile;
} PCB;

typedef struct PCBList {
	PCB* head;
	PCB* tail;
	int listSize;
} PCBList;

typedef struct dataIocpu {
	int pid;
	int cpuTime;
	int ioTime;
	char fileName[20];
} dataIocpu;

// message buffer that contains child process'super data.
struct msgBufIocpu {
	long mType;
	struct dataIocpu mData;
};

void mount(void);
void unmount(void);
void printRootDir(void);
int fileOpen(char* fileName, int mode);
int fileClose(char* fileName, int mode);
int fileWrite(PCB* fileDescriptor, char* bufferPointer);
int fileRead(PCB* fileDescriptor, char* bufferPointer, int readDataSize);
void randFileSelect(char* fileName);
int hashFun(char* fileName);

void signalTimeTick(int signo);
void signalRRcpuSchedOut(int signo);
void signalIoSchedIn(int signo);
void initPCBList(PCBList* list);
void pushPCB(PCBList* list, int procNum, int cpuTime, int ioTime, PCBfile file);
void popPCB(PCBList* list, PCB* runPCB);
void deletePCB(PCBList* list);
bool isEmptyList(PCBList* list);
void cMsgSndIocpu(int procNum, int cpuBurstTime, int ioBurstTime);
void pMsgRcvIocpu(int procNum, PCB* nodePtr);

int CPID[MAX_PROCESS];					// child process pid.
int KEY[MAX_PROCESS];					// key value for message queue.
int CONST_TICK_COUNT;
int TICK_COUNT;
int RUN_TIME;
int freeBlocks[4088];
int numFreeBlocks;

PCBList* waitQueue;
PCBList* readyQueue;
PCB* cpuRunPCB;
PCB* ioRunPCB;

FILE* pFileSystem;
FILE* pFileDump;
FILE* pBurst;
partition part;
dentry dirEntry;