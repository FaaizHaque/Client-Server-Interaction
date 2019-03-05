all: server client

server: server.c
		gcc -g -Wall -o server server.c -lrt -pthread
client: client.c
		gcc -g -Wall -o client client.c -lrt -pthread
clean:
		rm -fr s.out c.out *~ *.o

