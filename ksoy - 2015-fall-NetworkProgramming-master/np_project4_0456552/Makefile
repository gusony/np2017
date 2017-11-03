all: server.o common.o
	g++ server.o common.o -o server
	chmod 755 server

server.o: server.cpp common.h
	g++ -c server.cpp
common.o:common.cpp common.h
	g++ -c common.cpp

cgi: hw4.cpp
	g++ hw4.cpp -o hw4.cgi
	chmod 755 hw4.cgi

