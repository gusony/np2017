all:.single.o .socket.o .common.o
	gcc .single.o .socket.o .common.o -o .single-server

.single.o:.single.c .socket.h .common.h
	gcc -c .single.c
.socket.o:.socket.c .socket.h .common.h
	gcc -c .socket.c
.common.o:.common.c .common.h
	gcc -c .common.c

