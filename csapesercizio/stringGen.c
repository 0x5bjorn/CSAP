#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stringGen.h"

char **stringGen(char **p, int n, int len) 
{
    char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/,.-+=~`<>:";

    srand(getpid());
    for(int i = 0; i < n; i++) {
	int sz=rand()%len+1;
	if (!(p[i]=malloc(sz+1))) {
	    return(NULL);
	}
	bzero(p[i],sz+1);
	for (int j=0; j<sz; j++) {
	    p[i][j]=chars[rand()%sizeof(chars)-1];
	}
    }
    return(p);
}
