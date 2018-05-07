#include <stdio.h>
#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
#include <strings.h>

int main(int argc, char **argv)
{
	if(2 != argc)
	{
		printf("Usage: %s <file>\n", argv[0]);
		return 0;
	}

	int fd = open(argv[1], O_RDONLY);
	if(-1 == fd)
	{
		perror("open");
		return -1;
	}

	char buf[100] = {0};
	while(read(fd, buf, sizeof buf))
	{
		printf("%s", buf);
		bzero(buf, sizeof buf);
	}

	close(fd);
}
