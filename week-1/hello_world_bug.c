#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char* argv[]){
	char name[10];
	strcpy(name, argv[1]);
	printf("hello %s\n",name);
	return 0;
}

