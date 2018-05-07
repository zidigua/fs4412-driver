#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void key_down(int sig)
{
	printf("read --- \n");
}

void key_urg(int sig)
{
	printf("key_urg --- \n");
}

int main(int argc, char *argv)
{
	int fd1, fd2, fd3;
	int buf;

	fd1 = open("/dev/my_key2", O_RDWR);
	fd2 = open("/dev/my_key3", O_RDWR);
	fd3 = open("/dev/my_key4", O_RDWR);


	fd_set myfds;
	FD_ZERO(&myfds);
	FD_SET(fd1, &myfds);
	FD_SET(fd2, &myfds);
	FD_SET(fd3, &myfds);

	while(1)
	{
		fd_set tem = myfds;
		select(fd3+1, &tem, NULL, NULL, NULL);

		if(FD_ISSET(fd1, &tem))
		{
			read(fd1, &buf, sizeof(int));
			printf("read1: %d \n", buf);
		}

		if(FD_ISSET(fd2, &tem))
		{
			read(fd2, &buf, sizeof(int));
			printf("read2: %d \n", buf);
		}

		if(FD_ISSET(fd3, &tem))
		{
			read(fd3, &buf, sizeof(int));
			printf("read3: %d \n", buf);
		}
	}





	while(1);
	return 0;
}
