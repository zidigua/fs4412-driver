#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>

int main(int argc, char **argv)
{
	if(2 != argc)
	{
		printf("Usage: %s <file>\n", argv[0]);
		return 0;
	}

	int fd = open(argv[1], O_RDWR);
	if(-1 == fd)
	{
		perror("open");
		return -1;
	}

	char *p = mmap(NULL, 10240, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	int i;
	printf("%s", p);
	for(i=0;p[i];i++) {
		p[i]++;
	}


	close(fd);
	return 0;
}








		








