#include "FileSystem.h"

/*
*	void mount(void)
*	A function that copies the superBlock and Inode of disk.img to the memory partition
*	and outputs the contents to the File_dump.txt file.
*/
void mount(void) {
	pFileSystem = fopen("disk.img", "r+");
	pFileDump = fopen("FileDump.txt", "w");

	if (pFileSystem == NULL) {
		printf("There are no disk.img,\n");
		exit(0);
	}

	fread(&part.super, sizeof(superBlock), 1, pFileSystem);										//copy sizeof(superBlock) bytes to buffer<part.super>
	fprintf(pFileDump, "---------------------SuperBlock Contents---------------------\n");
	fprintf(pFileDump, "  Partition Type   - %d\n", part.super.partitionType);
	fprintf(pFileDump, "  Block Size       - %d bytes\n", part.super.blockSize);
	fprintf(pFileDump, "  Inode Size       - %d bytes\n", part.super.inodeSize);
	fprintf(pFileDump, "  First Inode      - %d\n", part.super.firstInode);
	fprintf(pFileDump, "  num Inodes       - %d\n", part.super.numInodes);
	fprintf(pFileDump, "  num Inode Blocks - %d\n", part.super.numInodeBlocks);
	fprintf(pFileDump, "  num Free Inodes  - %d\n", part.super.numFreeInodes);
	fprintf(pFileDump, "  num Blocks       - %d\n", part.super.numBlocks);
	fprintf(pFileDump, "  num Free Blocks  - %d\n", part.super.numFreeBlocks);
	fprintf(pFileDump, "  First Data Block - %d\n", part.super.firstDataBlock);
	fprintf(pFileDump, "  Volume Name      - %s\n", part.super.volumeName);
	fprintf(pFileDump, "-------------------------------------------------------------\n");

	for (int i = 0; i < 224; i++) {
		fread(&part.inodeTable[i], sizeof(inode), 1, pFileSystem);

		fprintf(pFileDump, "---------------------Inode[%3d] Contents---------------------\n", i);
		fprintf(pFileDump, "  Mode           - %d\n", part.inodeTable[i].mode);					//File type + Mode ex) 0x1:0777 = Reg(0x1:0000) + AC_ALL(0x777)
		fprintf(pFileDump, "  Locked         - %d\n", part.inodeTable[i].locked);
		fprintf(pFileDump, "  Date           - %d\n", part.inodeTable[i].date);
		fprintf(pFileDump, "  File Size      - %d bytes\n", part.inodeTable[i].size);
		fprintf(pFileDump, "  Indirect Block - %d\n", part.inodeTable[i].indirectBlock);
		fprintf(pFileDump, "  Blocks - ");
		for (int t = 0; t < 6; t++) {
			fprintf(pFileDump, "%d ", part.inodeTable[i].blocks[t]);
			freeBlocks[part.inodeTable[i].blocks[t]] = 1;
			if (part.inodeTable[i].blocks[t] != 0) {											//Not Count block[0]
				numFreeBlocks--;
			}	
		}
		fprintf(pFileDump, "\n-------------------------------------------------------------\n");
	}
	if (freeBlocks[0] == 1) {																	//Add Count block[0]
		numFreeBlocks--;
	}
	part.super.numFreeBlocks = numFreeBlocks;
	
	fclose(pFileDump);
}

/*
*	void unmount(void)
*	Function to copy and save superBlock and Inode contents of partition memory to disk.img.
*/

void unmount(void) {
	fseek(pFileSystem, 0, SEEK_SET);															//File Pointer Move to start point
	fwrite(&part.super, sizeof(superBlock), 1, pFileSystem);									//Update superBlock (part<memory> -> img<disk>)
	for (int i = 0; i < 224; i++) {																//Update inodeTable (part<momory> -> img<disk>)
		fwrite(&part.inodeTable[i], sizeof(inode), 1, pFileSystem);
	}
	printf("File System Unmount\n");
	fclose(pFileSystem);
}

/*
*	void printRootDir(void)
*	A function that outputs the contents of a file in the root directory to File_dump.txt.
*/
void printRootDir(void) {
	int blockNum = part.inodeTable[part.super.firstInode].size / BLOCK_SIZE;					//Block number = file size / block size
	int blockLocation;

	if ((part.inodeTable[part.super.firstInode].size % BLOCK_SIZE) != 0) {
		blockNum++;
	}

	pFileDump = fopen("FileDump.txt", "a+");

	fprintf(pFileDump, "-------------------------------------------------Root Directory------------------------------------------------\n");
	for (int i = 0, m = 1; i < blockNum; i++) {
		blockLocation = BLOCK_SIZE * (part.super.firstDataBlock + part.inodeTable[part.super.firstInode].blocks[i]);		//Directory Entry = Firstblock(8) + rootdir inode -> block
		fseek(pFileSystem, blockLocation, SEEK_SET);
		for (int t = 0; t < BLOCK_SIZE; t = t + 32, m++) {
			fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.dirLength, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.nameLen, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.fileType, sizeof(int), 1, pFileSystem);
			fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
			if (dirEntry.inode == 0) {																						//End -> stop printing
				break;
			}
			fprintf(pFileDump, "%15s ", dirEntry.name);
			if (m % 7 == 0) {																								//Every 7 print -> new line
				fprintf(pFileDump, "\n");
			}
		}
	}
	fprintf(pFileDump, "\n---------------------------------------------------------------------------------------------------------------\n");

	fclose(pFileDump);
}

/*
*	int findFreeBlock(void)
*	A function that finds an empty data block and returns a number.
*/
int findFreeBlock(void) {
	int free;
	for (int i = 0; i < 4088; i++) {
		if (freeBlocks[i] == 0) {
			free = i;
			return free;
		}
	}
}

/*
*	int fileOpen(char* fileName, int mode)
*	A function that opens a file named char* fileName as input.
*	It checks the permission according to the mode and locks the file when it is opened in write mode.
*/
int fileOpen(char* fileName, int mode) {
	int inodeNum = hashFun(fileName);
	int permission;

	if (inodeNum == -1) {
		printf("There are no file : %s\n", fileName);
		return 1;
	}

	permission = part.inodeTable[inodeNum].mode & 0xFFF;								//mode = File type + Permission(0xFFF)
	
	if (mode == 1) {																	//mode 1 -> read mode
		if (permission == 0x777 || permission == 0x1) {
			printf("File [%s] Open as Read Mode\n",fileName);
			return 0;
		}
		else {
			printf("Permission Deny\n");
			return 1;
		}
	}
	else if (mode == 2) {																//mode 2 -> write mode
		if (part.inodeTable[inodeNum].locked == 1) {
			printf("File [%s] Open by other processor\n", fileName);
			return 1;
		}
		if (permission == 0x777 || permission == 0x2) {
			part.inodeTable[inodeNum].locked = 1;										//Update inode Lock condition(part<memory>)
			printf("File [%s] Open as Write Mode\n",fileName);
			return 0;
		}
		else {
			printf("Permission Deny\n");
			return 1;
		}
	}
	else {
		printf("Wrong Mode : File Open Fail\n");
		return 1;
	}
}

/*
*	int fileClose(char* fileName, int mode)
*	A function that closes the input file named char* fileName. 
*	Unlocks the file opened in write mode.
*/
int fileClose(char* fileName, int mode) {
	int inodeNum = hashFun(fileName);
	int inodeLocation;

	if (inodeNum == -1) {
		printf("There are no file : %s\n", fileName);
		return 1;
	}

	if (mode == 2) {																		//mode 2 -> write mode
		part.inodeTable[inodeNum].locked = 0;												//Update inode Lock condition(part<memory>)
	}
	printf("File [%s] Close\n", fileName);
	return 0;
}

/*
*	void randFileSelect(char* fileName)
*	A function that selects a random file from the root directory and returns the name of the file.
*/
void randFileSelect(char* fileName) {
	int fileNum = part.inodeTable[part.super.firstInode].size / 32;							//Directroy Entry = 32 bytes
	int randFile;
	int fileLocation;
	//int blockNum;

	//======Random Seed reset========
	randFile = rand();
	srand(getpid() + time(NULL) + randFile);
	//===============================
	randFile = (rand() % (fileNum - 2)) + 2;												//1, 2 is ".", ".." file -> skip, only use file_0 ~ 100 (inode 2~102)
	fileLocation = (BLOCK_SIZE * part.super.firstDataBlock) + (32 * randFile);				//firstDataBlock(8) -> Block[0], Block[0~3] -> root dir entry
	fseek(pFileSystem, fileLocation, SEEK_SET);
	fread(&dirEntry.inode, sizeof(int), 1, pFileSystem);
	fread(&dirEntry.dirLength, sizeof(int), 1, pFileSystem);
	fread(&dirEntry.nameLen, sizeof(int), 1, pFileSystem);
	fread(&dirEntry.fileType, sizeof(int), 1, pFileSystem);
	fread(&dirEntry.name, sizeof(char), 16, pFileSystem);
	strcpy(fileName, dirEntry.name);
}

/*
*	int fileWrite(PCB* fileDescriptor, char* bufferPointer)
*	A function that puts data into the file requested by the process.
*	If the data you input exceeds the data blocks originally allocated for the file inode,
*	additional blocks of data are allocated.
*/
int fileWrite(PCB* fileDescriptor, char* bufferPointer) {
	int inodeNum = hashFun(fileDescriptor->openFile.fileName);
	char writeBuffer[BLOCK_SIZE];
	int bufferBlock;
	int blockLocation;

	bufferBlock = (strlen(bufferPointer) / BLOCK_SIZE) + 1;										//Write Data blocks = Write Data size / Block size
	part.inodeTable[inodeNum].size = (strlen(bufferPointer) + 1);								//Write Data Size Update (part<memory>)

	if (inodeNum == -1) {
		printf("There are no file : %s\n", fileDescriptor->openFile.fileName);
		return 1;
	}

	for (int i = 0; i < bufferBlock; i++) {
		if (part.inodeTable[inodeNum].blocks[i] == 0) {
			part.inodeTable[inodeNum].blocks[i] = findFreeBlock();
		}
		blockLocation = BLOCK_SIZE * (part.super.firstDataBlock + part.inodeTable[inodeNum].blocks[i]);
		fseek(pFileSystem, blockLocation, SEEK_SET);
		bufferPointer += BLOCK_SIZE * i;																						//String Pointer Move 1024 * block bytes
		strcpy(writeBuffer, bufferPointer);
		fwrite(writeBuffer, sizeof(blocks), 1, pFileSystem);
	}
	printf("Data [%s] write to File [%s] success\n", writeBuffer, fileDescriptor->openFile.fileName);
	return 0;
}

/*
*	int fileRead(PCB* fileDescriptor, char* bufferPointer, int readDataSize)
*	A function that reads as much data as requested from the file requested by the process. 
*	It returns as much as the size of the data actually read.
*/
int fileRead(PCB* fileDescriptor, char* bufferPointer, int readDataSize) {
	int inodeNum = hashFun(fileDescriptor->openFile.fileName);
	int offSetBlock = fileDescriptor->openFile.offSet / BLOCK_SIZE;																//offSet block = Last Read Point Block
	int offSet = fileDescriptor->openFile.offSet % BLOCK_SIZE;																	//offSet = Last Read Point
	int blockLocation = BLOCK_SIZE * (part.super.firstDataBlock + part.inodeTable[inodeNum].blocks[offSetBlock]);				//file Block location = First Data Block(8) + file inode.blocks[offSet block] * 1024bytes
	int readByte;

	fseek(pFileSystem, blockLocation + offSet, SEEK_SET);

	fgets(bufferPointer, readDataSize, pFileSystem);
	readByte = strlen(bufferPointer);
	for (int i = 0; i < readByte; i++) {																						//\n scan
		if (bufferPointer[i] == '\n') {																							//change \n -> \t for print format
			bufferPointer[i] = '\t';
		}
	}
	fileDescriptor->openFile.offSet += readByte;																				//offSet Update
	return readByte;
}

/*
*	int hashFun(char* fileName)
*	A hash function that returns a hash value with the file name as the key value.
*/
int hashFun(char* fileName) {
	int hash;
	char buffer[20];
	char* fNameBuffer;
	int fNumBuffer;
	
	strcpy(buffer, fileName);

	fNameBuffer = strtok(buffer, "file_");										//buffer(file_num), fNamebuffer point "num"
	if (fNameBuffer == NULL) {
		printf("Hash Error\n");
		return -1;																//No hash -> No File
	}
	fNumBuffer = atoi(fNameBuffer);												//"num"<string> -> num<int>
	return fNumBuffer + 2;														//return inode number
}

