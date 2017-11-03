all:single.o socket.o common.o
	g++ single.o socket.o common.o -o single-server

single.o:single.cpp socket.h common.h
	g++ -c single.cpp
socket.o:socket.cpp socket.h common.h
	g++ -c socket.cpp
common.o:common.cpp common.h
	g++ -c common.cpp

