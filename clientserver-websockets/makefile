CFLAGS = -Wall -g

client : client.o lib.o
	cc -o client client.o lib.o

server : server.o lib.o
	cc -o server server.o lib.o

lib : lib.o
	cc -o lib lib.o

lib.o : lib.h

server.o : lib.h

client.o : lib.h

clean : 
	rm client client.o server server.o lib lib.o

