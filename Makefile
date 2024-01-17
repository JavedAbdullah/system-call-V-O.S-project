

CC = gcc
CFLAGS = -c -Wall

all:
	 $(CC)  ./src/F4server.c ./src/shared_memory.c ./src/semaphore.c  ./src/errExit.c  -o ./F4Server
	$(CC) ./src/F4Client.c ./src/shared_memory.c ./src/semaphore.c ./src/errExit.c   -o ./F4Client

clean:
	rm ./F4Server 
	rm ./F4Client
