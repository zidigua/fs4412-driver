#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>


int main(int argc, char *argv[])
{

	int num;
	int fd = open("/dev/hello", O_RDONLY);
	if(fd < 0) {
		perror("open");
		return -1;
	}

	while(read(fd, &num, 10)) {

		printf("%#x \n", num);
	}

	return 0;
}
