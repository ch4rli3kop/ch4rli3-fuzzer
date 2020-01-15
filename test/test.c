#include <stdio.h>

int main(int argc, char* argv[]){

	char buff[0x10];
	char* file = argv[1];
	FILE *f;
	int cnt;

	printf("test program start!\n");
	printf("read file : %s\n", argv[1]);
	f = fopen(file, "r");

	for (int i=0; fread(buff+i, sizeof(char), 1, f) > 0; i++);
	fclose(f);

	printf("buff : %s\n", buff);
	return 0;
}
