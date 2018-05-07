#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
/*
void *start_routine(void *arg) {
	int fd = open("/dev/hello", O_RDONLY);
	if(fd < 0) {
		perror("open");
		return -1;
	}

	while(read(fd, &num, 10)) {

		//printf("%#x \n", num);
	}

}

*/
int main(int argc, char *argv[])
{

	int num;
	int i = 0;
/*	for(; i < 10000; i++) {
		pthread_t *thread;
		pthread_create(thread, NULL, start_routine, NULL);
	}
*/

	while(read(fd, &num, 10)) {

		printf("%#x \n", num);
	}

	while(1);
	return 0;
}
