#include "FileSystem.h"

void initPartition(void);
int findFreeInode(void);
void save2File(void);
void makeFile(void);
void printFileInfo(void);

FILE* pMakeFile;
int freeInode[224];
int numFreeInode;

int main(void) {
	int menuSelect;
	pMakeFile = fopen("disk.img", "w");
	memset(&part, 0, sizeof(partition));
	memset(freeBlocks, 0, sizeof(freeBlocks));
	memset(freeInode, 0, sizeof(freeInode));
	numFreeBlocks = 4088;
	numFreeInode = 224;

	initPartition();

	while (1) {
		printf("----------------File Maker----------------\n");
		printf("1. Make file\n");
		printf("2. Make Dump file\n");
		printf("3. Exit\n");
		printf("-------------------------------------------\n");

		scanf("%d", &menuSelect);

		switch (menuSelect) {
		case 1:
			system("clear");
			makeFile();
			save2File();
			break;
		case 2:
			mount();
			printRootDir();
			printFileInfo();
			break;
		case 3:
			save2File();
			fclose(pMakeFile);
			return 0;
			break;
		default:
			break;
		}

	}
}

/*
*	void initPartition(void)
*	Function to initialize the superBlock of the partition memory and the root directory Inode.
*/
void initPartition(void) {
	//superBlock Setting
	part.super.partitionType = 0x1111;						//SIMPLE_PARTITON
	part.super.blockSize = 1024;
	part.super.inodeSize = 32;
	part.super.firstInode = 2;
	part.super.numInodes = 224;
	part.super.numInodeBlocks = 7;
	part.super.numFreeInodes = 224;
	part.super.numBlocks = 4088;
	part.super.numFreeBlocks = 4088;
	part.super.firstDataBlock = 8;
	strcpy(part.super.volumeName, "TAKPAK_Partition");

	//inodeTable[0] Setting
	freeInode[0] = 1;
	numFreeInode--;
	part.inodeTable[0].size = 4186112;
	//inodeTable[1] Setting
	freeInode[1] = 1;
	numFreeInode--;
	//inodeTable[2] Setting
	part.inodeTable[part.super.firstInode].mode = 0x20777;	//DIR_FILE + AC_ALL
	part.inodeTable[part.super.firstInode].size = 64;
	//firstBlock Setting
	freeBlocks[0] = 1;
	numFreeBlocks--;

	//dentry[0] Setting "."
	memset(&dirEntry, 0, sizeof(dentry));
	dirEntry.inode = 1;
	dirEntry.dirLength = 32;
	dirEntry.nameLen = 2;
	dirEntry.fileType = 1;
	strcpy(dirEntry.name, ".");
	memcpy(part.dataBlocks[0].d, &dirEntry, 32);			//Copy dentry[0] to dataBlock[0]

	//dentry[1] Setting ".."
	memset(&dirEntry, 0, sizeof(dentry));
	freeInode[2] = 1;
	numFreeInode--;
	dirEntry.inode = 2;
	dirEntry.dirLength = 32;
	dirEntry.nameLen = 3;
	dirEntry.fileType = 1;
	strcpy(dirEntry.name, "..");
	memcpy(part.dataBlocks[0].d + 32, &dirEntry, 32);		//Copy dentry[1] to dataBlock[0]
}

/*
*	int findFreeInode(void)
*	A function that searches for an empty inode and returns a number.
*/
int findFreeInode(void) {
	int free;
	for (int i = 0; i < 224; i++) {
		if (freeInode[i] == 0) {
			free = i;
			return free;
		}
	}
}

/*
*	void makeFile(void)
*	A function that creates a file in the root directory.
*/
void makeFile(void) {
	char fileName[16];
	char fileContents[1024];
	int blockNum;
	int newInode;
	int newBlock;

	system("clear");
	printf("----------------File Make-----------------\n");
	printf("Enter File name(Max. 15 char) : ");
	__fpurge(stdin);
	gets(fileName);
	fileName[15] = '\0';															//Exception Handlr
	__fpurge(stdin);																//stdin buffer flush
	printf("File name : %15s\n", fileName);
	printf("Enter File contents (Max. 1023 char)\n->");
	gets(fileContents);
	fileContents[1023] = '\0';
	__fpurge(stdin);

	blockNum = part.inodeTable[part.super.firstInode].size / 1024;
	if ((part.inodeTable[part.super.firstInode].size % 1024) == 0) {				//if root dirEntry need new block
		part.inodeTable[part.super.firstInode].blocks[blockNum] = findFreeBlock();
	}

	//super block update
	part.inodeTable[part.super.firstInode].size += 32;
	//file Inode update
	newInode = findFreeInode();														//new File inode number
	freeInode[newInode] = 1;
	numFreeInode--;
	//root dirtory update
	memset(&dirEntry, 0, sizeof(dentry));
	dirEntry.inode = newInode;
	dirEntry.dirLength = 32;
	dirEntry.nameLen = strlen(fileName);
	dirEntry.fileType = 1;
	strcpy(dirEntry.name, fileName);
	if ((part.inodeTable[part.super.firstInode].size % 1024) == 0) {
		memcpy((part.dataBlocks[blockNum].d + part.inodeTable[part.super.firstInode].size), &dirEntry, 32);
		//root dir entry			+		entry num * 32
	}
	else {
		memcpy((part.dataBlocks[blockNum].d + part.inodeTable[part.super.firstInode].size - 32), &dirEntry, 32);
		//root dir entry			+		entry num * 32
	}


	//file Inode update
	part.inodeTable[newInode].mode = 0x10000 + 0x777;			//REG_FILE + AC_ALL
	part.inodeTable[newInode].locked = 0;
	part.inodeTable[newInode].date = 0;
	part.inodeTable[newInode].size = strlen(fileContents);
	part.inodeTable[newInode].indirectBlock = -1;

	//Data Block update
	newBlock = findFreeBlock();
	freeBlocks[newBlock] = 1;
	numFreeBlocks--;
	part.inodeTable[newInode].blocks[0] = newBlock;
	strcpy(part.dataBlocks[newBlock].d, fileContents);
	//file inode.block[0].d     cpy	 fileContents
}

/*
*	void save2File(void)
*	A function that writes data from the partition memory to the disk.img file.
*/
void save2File(void) {
	fseek(pMakeFile, 0, SEEK_SET);																//File Pointer Move to start point
	part.super.numInodes = numFreeInode;
	part.super.numFreeBlocks = numFreeBlocks;
	fwrite(&part.super, sizeof(superBlock), 1, pMakeFile);										//Update superBlock (part<memory> -> img<disk>)
	for (int i = 0; i < 224; i++) {																//Update inodeTable (part<momory> -> img<disk>)
		fwrite(&part.inodeTable[i], sizeof(inode), 1, pMakeFile);
	}
	for (int i = 0; i < 4088; i++) {
		fwrite(&part.dataBlocks[i], sizeof(blocks), 1, pMakeFile);
	}
}

/*
*	void printFileInfo(void)
*	A function that prints the names and contents of files in the root directory.
*/
void printFileInfo(void) {
	int blockNum = part.inodeTable[part.super.firstInode].size / BLOCK_SIZE;					//Block number = file size / block size
	int blockLocation;
	int inodeNum;
	int dataNum;

	pFileDump = fopen("FileDump.txt", "a+");

	if ((part.inodeTable[part.super.firstInode].size % BLOCK_SIZE) != 0) {
		blockNum++;
	}

	fprintf(pFileDump, "----------------------------------------------------File Info---------------------------------------------------\n");
	for (int i = 0; i < blockNum; i++) {
		blockLocation = BLOCK_SIZE * (part.super.firstDataBlock + part.inodeTable[part.super.firstInode].blocks[i]);			//Directory Entry = Firstblock(8) + rootdir inode -> block
		fseek(pFileSystem, blockLocation, SEEK_SET);
		for (int t = 0; t < BLOCK_SIZE; t = t + 32) {
			fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
			fseek(pFileSystem, 12, SEEK_CUR);																					//Move File pointer 12 bytes ->useless data
			fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
			if (dirEntry.inode == 0) {																							//End -> stop printing
				break;
			}
			fprintf(pFileDump, "File Name : %15s\n", dirEntry.name);
			//File info
			inodeNum = dirEntry.inode;
			dataNum = part.inodeTable[inodeNum].blocks[0];
			fprintf(pFileDump, "File Contents : %s\n", part.dataBlocks[dataNum].d);
		}

	}
	fprintf(pFileDump, "\n--------------------------------------------------------------------------------------------------------------\n");

	fclose(pFileDump);
}
