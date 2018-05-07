#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, const char *argv[])
{
	int fd = open("/dev/led", O_RDWR);
	if(fd < 0) {
		perror("open");
		return -1;
	}
#define ON 0
#define OFF 1
	
	char *addr = mmap(NULL, 10240, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if(MAP_FAILED == addr) {
		perror("mmap");
		return -1;
	}
	
	int *led_dat_reg = (int *)(addr + 0x0c44);
	while(1) {
		/*
		ioctl(fd, ON);
		sleep(1);
		ioctl(fd, OFF);
		sleep(1);
		*/
		*led_dat_reg = 1 << 7;
		sync();
		sleep(1);
		*led_dat_reg &= ~(1 << 7);
		sync();
		sleep(1);
	}

	return 0;
}
