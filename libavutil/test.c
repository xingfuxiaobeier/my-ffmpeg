#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FILE_NAME_SIZE 1024
int main(int argc, char const *argv[])
{
	char* test = (char*) malloc(sizeof(char) * FILE_NAME_SIZE);
	char test2[] = "/storage/emulated/0/aweidasheng/asset/ybbz/asset1.mp4";
	char test1[FILE_NAME_SIZE];
	sprintf(test1, "%s", test2);

	printf("before test : %s\n", test);
	printf("before test1 : %s\n", test1);
	memcpy(test, test1, FILE_NAME_SIZE);

	printf("after test : %s\n", test);
	printf("after test1 : %s\n", test1);
	return 0;
}