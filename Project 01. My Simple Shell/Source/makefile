CC = gcc
TARGET = SimpleShell.out
OBJS = SimpleShellMain.o SimpleShellFunction.o

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

main.o :
	$(CC) -c SimpleShellMain.c

function.o :
	$(CC) -c SimpleShellFunction.c

clean :
	rm -f $(OBJS) $(TARGET)
