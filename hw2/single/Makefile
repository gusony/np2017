all:single.o socket.o common.o
	g++ single.o socket.o common.o -o single-server -Wall

single.o:single.c socket.h common.h
	gcc -c single.c -Wall
socket.o:socket.c socket.h common.h
	gcc -c socket.c -Wall
common.o:common.c common.h
	gcc -c common.c -Wall

