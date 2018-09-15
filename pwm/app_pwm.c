#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd = 0; 
	int flag = 11;
	fd = open("/dev/pwm", O_RDWR);
	if (fd == -1) {
		perror("open");
		return fd;
	}

	ioctl(fd, 10, flag);
	while(1);

	return 0;
}
