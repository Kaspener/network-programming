server: server.o socketFunctions.o
	gcc server.o socketFunctions.o -o server

client: client.o socketFunctions.o
	gcc client.o socketFunctions.o -o client

server.o: server.c
	gcc -c server.c

client.o: client.c
	gcc -c client.c

socketFunctions.o: socketFunctions.c
	gcc -c socketFunctions.c

clean:
	rm -rf *.o server client