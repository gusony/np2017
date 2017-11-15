all:concurrent.o socket.o common.o semaphore.o
	g++ concurrent.o socket.o common.o semaphore.o -o multi-server

concurrent.o:concurrent.cpp socket.h common.h semaphore.h
	g++ -c concurrent.cpp
socket.o:socket.cpp socket.h common.h
	g++ -c socket.cpp
common.o:common.cpp common.h
	g++ -c common.cpp
semaphore.o:semaphore.cpp semaphore.h common.h
	g++ -c semaphore.cpp

clear:
	ipcrm -M 31464
	ipcrm -M 31465
	ipcrm -M 31466
	ipcrm -M 31467
	ipcrm -M 18147
	ipcrm -M 18148
	ipcrm -M 18149
	ipcrm -M 18150
	ipcrm -S 77320
	ipcrm -S 77321
	ipcrm -S 77322
	ipcrm -S 77323
