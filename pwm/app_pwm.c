#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd = open("pwm", O_RDWR);
	
	int flag = 11;

	ioctl(fd, 10, flag);


	return 0;
}
