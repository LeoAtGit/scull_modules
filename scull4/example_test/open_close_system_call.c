#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

int main (int argc, char *argv[])
{
	char *filename = NULL;
	int fd = 0;

	if (argc != 2){
		printf("wrong usage, give me a filename\n");
		return -1;
	}

	filename = argv[1];

	fd = open(filename, O_RDONLY);
	if (fd == -1){
		printf("couldnt open %s\n", filename);
		return -2;
	}

	close(fd);

	return 0;
}

