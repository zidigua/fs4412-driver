#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/input.h>

int main(void)
{
	int fd;
	int ret;
	struct input_event event;
	
	fd = open("/dev/input/event1", O_RDWR);
	if(fd<0) {
		perror("open");
		exit(1);
	}

	while(1) {
		ret = read(fd, &event, sizeof(struct input_event));
		if (ret < 0) {
			perror("read");
			exit(1);
		}

		if(event.type == EV_ABS) {
			
			if (event.code == ABS_RX) {
				printf("__APP_USER__: ABS_RX  %d ", event.value);
			} else if (event.code == ABS_RY) {
				printf(" ABS_RY  %d ", event.value);
			} else if (event.code == ABS_RZ) {
				printf(" ABS_RZ  %d \n", event.value);
			}
		}
	}

	close(fd);
	return 0;
}



