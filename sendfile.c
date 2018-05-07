#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd = open("sendfile.c", O_RDONLY);
	int fdout = open("aaa.txt", O_WRONLY|O_CREAT, 0666);
	perror("open");
	struct stat filestat;
	fstat(fd, &filestat);
	

	int a = sendfile(fdout, fd, NULL, filestat.st_size);	
	perror("sendfile");
	printf("%d \n", a);
	return 0;
}
