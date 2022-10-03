all : FileSystem.out ImgMaker.out

FileSystem.out : RoundRobin.o FileSystem.o Schedule.o
	gcc -o FileSystem.out RoundRobin.o FileSystem.o Schedule.o

ImgMaker.out : ImgMaker.o FileSystem.o
	gcc -o ImgMaker.out ImgMaker.o FileSystem.o

RoundRobin.o : RoundRobin.c
	gcc -c -o RoundRobin.o RoundRobin.c

FileSystem.o : FileSystem.c
	gcc -c -o FileSystem.o FileSystem.c

Schedule.o : Schedule.c
	gcc -c -o Schedule.o Schedule.c

ImgMaker.o : ImgMaker.c
	gcc -c -o ImgMaker.o ImgMaker.c

clean :
	rm *.o
