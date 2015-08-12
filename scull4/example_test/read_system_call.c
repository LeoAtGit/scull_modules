#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

int main (int argc, char *argv[])
{
	char *filename = NULL;
	int fd = 0;
	int count = 0;

	char *buffer = malloc(1000); // magic number TODO buffer overflow watch out

	if (argc < 2){
		printf("wrong usage, %s filename [count]\n", argv[0]);
		return -1;
	}

	filename = argv[1];
	count = (argc == 3) ? atoi(argv[2]) : 100;

	fd = open(filename, O_RDWR);

	if (fd == -1){
		printf("couldnt open the file\n");
		return -2;
	}

	if (read(fd, buffer, count) < 0){
		printf("couldnt read anything m8\n");
		return -3;
	}

	printf("%s\n", buffer);

	close(fd);
	return 0;
}
