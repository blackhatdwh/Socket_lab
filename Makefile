#
# Makefile
# weihao, 2017-10-17 18:45
#

all:
	make client; make server;

client: client.o common.o
	gcc client.o common.o -o client -g -lpthread

server: server.o common.o
	gcc server.o common.o -o server -g -lpthread

client.o: client.c common.c common.h
	gcc -c client.c common.c

server.o: server.c common.c common.h
	gcc -c server.c common.c

common.o: common.c common.h
	gcc -c common.c

clean:
	rm client.o server.o client server commom.o

	

# vim:ft=make
#
