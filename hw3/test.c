#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void){
    FILE* fp = fopen("t6.txt","r");
    char c[10000];
    int i=60;
    int count=0;
    int t;
    do{
    	printf("---%d---\n",count++);
    	bzero(c,sizeof(c));
    	//if(fgets(c,sizeof(c),fp) != NULL){
    	fgets(c,sizeof(c),fp);
	        printf("len=%ld,%d,%d\n",strlen(c),c[strlen(c)-1],c[strlen(c)]);
	        while(c[strlen(c)] == 0 && c[strlen(c)-1] == 10 && c[strlen(c)-2] == 32){
	        	t = strlen(c);
	        	c[t-1] =0;
	        	c[t-2] =10;
	        }
	        printf("len=%ld,\n",strlen(c));
        //}
        printf("-----\n");

    }while(i--);
    return(0);
}

