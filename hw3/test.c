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

#define HOST_LEN 50
#define PORT_FILE_LEN 10
#define URL_LEN 2000
#define BUF_LEN 10000

void replace_special_char(char *mes_buf){
	char cp[BUF_LEN];
	strcpy(cp,mes_buf);

	char result[BUF_LEN];
	bzero(result,BUF_LEN);

	for(int i=0; i<strlen(cp); i++){
		if(cp[i] == '<')
			strcat(result,"&lt;");
		else if(cp[i] == '>')
			strcat(result,"&gt;");
		else if(cp[i] == '\"')
			strcat(result,"\\\"");
		else if(cp[i] == '\n')
			strcat(result,"<br>");
		else if(cp[i] == '\r')
			strcat(result,"");
		else
			result[strlen(result)] = cp[i];
	}
	strcpy(mes_buf,result);
}

int main(void){
	char *mes_buf=malloc(sizeof(char)*BUF_LEN);
	strcpy(mes_buf, "<!test.html> \"hello world\"\r\n\n");
	printf("before=%s,\n",mes_buf);
	replace_special_char(mes_buf);
	printf("after =%s,\n",mes_buf);
	return(0);
}