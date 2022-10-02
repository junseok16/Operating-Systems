all : VirtualMemory.out

VirtualMemory.out:VirtualMemory.o
	gcc -o VirtualMemory.out VirtualMemory.o

VirtualMemory.o:VirtualMemory.c
	gcc -c -o VirtualMemory.o VirtualMemory.c

clean :
	rm *.o
