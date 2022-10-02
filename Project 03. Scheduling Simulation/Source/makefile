all : FirstInFirstOut.out RoundRobin.out VirtualRoundRobin.out

FirstInFirstOut.out:FirstInFirstOut.o
	gcc -o FirstInFirstOut.out FirstInFirstOut.o
RoundRobin.out:RoundRobin.o
	gcc -o RoundRobin.out RoundRobin.o
VirtualRoundRobin.out:VirtualRoundRobin.o
	gcc -o VirtualRoundRobin.out VirtualRoundRobin.o


FirstInFirstOut.o:FirstInFirstOut.c
	gcc -c -o FirstInFirstOut.o FirstInFirstOut.c
RoundRobin.o:RoundRobin.c
	gcc -c -o RoundRobin.o RoundRobin.c
VirtualRoundRobin.o:VirtualRoundRobin.c
	gcc -c -o VirtualRoundRobin.o VirtualRoundRobin.c

clean :
	rm *.o
