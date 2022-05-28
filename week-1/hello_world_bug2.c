#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char* argv[]){
	char* name = malloc(10);
	strncpy(name, argv[1],9);
	printf("hello %s\n",name);
	return 0;
}

