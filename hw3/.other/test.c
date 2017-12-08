#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

int readline(int fd, char *ptr, int maxlen){// read from file
    int n=0,rc;
    char c;

    for(n=0; n<maxlen;n++){

        if((rc=read(fd, &c, 1))==1){
            if(c=='\0')
                break;
            *ptr++ = c;
        }
        else if (rc==0){
            if(n==1) return(0);
            else break;
        }
        else
            return(-1);
    }
    *ptr = 0;
    return(n);//string length
}
int main(void){
	char test[100];
	int a = readline(5,test,100);
	printf("%d\n",a);
}