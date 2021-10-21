COMPILE

	gcc client.c -o client
	gcc -pthread server.c -o server
	
	
BEFORE RUNNING

My program uses process id to create message queue keys and shared memory keys.

If there is a shared memory created before with the same key and if the user pass array arguments with sizes more than the free space, the program will fail.

It is not required but I recommend running these two commands before running client and server programs.

These will delete all message queues and shared memories.

	ipcrm --all=shm
	ipcrm --all=msg
	
The program will delete the queues and shared memories it creates after client finishes its job.
	

RUN

	./server
	./client [array1.txt] [array2.txt]
