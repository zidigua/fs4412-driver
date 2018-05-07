#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

int main(int argc, const char *argv[])
{
	int fd = open("fifo", O_RDWR);
	if(fd < 0) {
		perror("open");
		return fd;
	}
	int ret;
	char buf[1000];
	while(1) {
		memset(buf, 0, sizeof buf);
		ret = read(fd, buf, sizeof buf);
		if(ret < 0) {
			perror(" read ");
		}
		if(ret > 0) {
			printf(" app :< %s >\n", buf);
		}
		sleep(1);
	}

	return 0;
}
