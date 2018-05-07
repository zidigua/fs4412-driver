#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv)
{
	if(2 != argc)
	{
		printf("Usage: %s <file>\n", argv[0]);
		return 0;
	}

	int fd = open(argv[1], O_WRONLY);
	if(-1 == fd)
	{
		perror("open");
		return -1;
	}

	char buf[100];
	memset(buf, 'a', 100);
	buf[0] = 'X';
	buf[99] = 'X';
	while(write(fd, buf, 100)){
		sleep(1);
	}

	close(fd);
}
