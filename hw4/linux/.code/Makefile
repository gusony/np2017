all: server.o common.o
	g++ server.o common.o -o proxy_server_K
	chmod 755 proxy_server_K

server.o: server.cpp common.h
	g++ -c server.cpp
common.o:common.cpp common.h
	g++ -c common.cpp

#cgi: hw4.cpp
	#g++ hw4.cpp -o hw4.cgi
	#chmod 755 hw4.cgi

