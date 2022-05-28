#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void leak() {
	char* old_val = malloc (10);
	char* new_val = malloc (10);
	old_val = new_val;
	free(new_val);
	free(old_val);
	return;
}

int main() {
	leak();
	return 0;
}
