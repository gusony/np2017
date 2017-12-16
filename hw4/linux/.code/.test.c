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

#define HOST_LEN 50
#define PORT_LEN 10
#define FILE_LEN 10
#define URL_LEN 2000
#define BUF_LEN 40000
#define MAX_CLI_NUM 5

const char *error_file = "Error : '%s' doesn't exist<br>";

typedef enum {
	F_READ,
	F_WRITE,
}status_code;


char ip       [MAX_CLI_NUM][HOST_LEN];
char port     [MAX_CLI_NUM][PORT_LEN];
char file     [MAX_CLI_NUM][FILE_LEN];
char proxyip  [MAX_CLI_NUM][HOST_LEN];
char proxyport[MAX_CLI_NUM][PORT_LEN];

FILE *fp[MAX_CLI_NUM];

int main(void){
	struct hostent *he;
	struct sockaddr_in client_sin;
	struct sockaddr_in proxy_addr;
	int    client_fd;
	int one = 1;



	client_fd = socket(AF_INET,SOCK_STREAM,0);


	he=gethostbyname("127.0.0.1");
	bzero(&client_sin,sizeof(client_sin));
	client_sin.sin_family = AF_INET;
	client_sin.sin_addr = *((struct in_addr *)he->h_addr);
	client_sin.sin_port = htons((u_short)atoi("7788"));

	printf("%s\n",client_sin.sin_addr);

	return(0);
}