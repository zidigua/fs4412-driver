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

	int fd = open(argv[1], O_RDONLY);
	if(-1 == fd)
	{
		perror("open");
		return -1;
	}

// void *mmap(void *addr, size_t length, int prot, int flags,
//                  int fd, off_t offset);


	char *p = mmap(NULL, 10240, PROT_READ, MAP_SHARED, fd, 0);

	printf("%s", p);

	close(fd);
}
