#include <stdio.h>
#include <stdlib.h>

char* leak() {
	printf("Leak function called!");
	char* p = malloc(20);
	return p;
}

int main() {
	leak();
	return 0;
}
