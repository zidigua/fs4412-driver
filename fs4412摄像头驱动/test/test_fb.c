#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <linux/fb.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define LEN 576000
struct RGB_COMB {
	unsigned char RGB_h;
	unsigned char RGB_l;
};
struct videobuffer{
	void * start;
	int length;
};

int x,y,i=0;
struct videobuffer *buffer;
int fbfd;
unsigned int screen_size;
char *  virtual_addr;
struct fb_var_screeninfo varinfo;
struct RGB_COMB new_rgb;
unsigned short rgb;
char buff[LEN];

int open_camera_driver(void)
{
	int fd;
	fd = open("/dev/video0",O_RDWR);
	return fd;
}

int open_lcd_driver()
{
	fbfd = open("/dev/fb0",O_RDWR);
	if(fbfd < 0){
		perror("open lcd file is fail.");
		return 0;
	}
}

int init_lcd_attitude()
{
	int ret;
	ret = ioctl(fbfd,FBIOGET_VSCREENINFO,&varinfo);
	if(ret != 0){
		perror("get fbinfo var is fail.");
	}
	
	printf("xres: %d,yres: %d,bits_per_pixel: %d\n",varinfo.xres,varinfo.yres,varinfo.bits_per_pixel);
	
	screen_size = varinfo.xres * varinfo.yres * varinfo.bits_per_pixel / 8;
	
	printf("screen_size : %d \n",screen_size);

	virtual_addr = (char *)mmap(0, screen_size, PROT_READ|PROT_WRITE, MAP_SHARED,fbfd,0);
	if((int)virtual_addr == -1){
		perror("alloc virtual address is fail.");
		return 0;
	}
	printf("virtual_addr : %p\n",virtual_addr);
}

int init_camera_attitude(int fd)
{
	int i;
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer v4l2_buf;
	//1.查询驱动
	if(ioctl(fd,VIDIOC_QUERYCAP,&cap) == -1){
		return -1;
	}
	if(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE){
		printf("check camera is success.\n");
	}else{
		return -1;
	}
	//2.设置图像格式
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 600;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if(ioctl(fd,VIDIOC_S_FMT,&fmt) == -1){
		return -1;
	}
	
	//3.申请缓冲区
	reqbuf.count = 4;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;

	if(ioctl(fd,VIDIOC_REQBUFS,&reqbuf) == -1){
		return -1;
	}
}

int start_capturing(int fd)
{
	//7.启动摄像头
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(fd,VIDIOC_STREAMON,&type) == -1){
		return -1;
	}

}


int read_data(int fd)
{
	int ret;
	ret = read(fd,buff,LEN);
	if(ret == -1){
		printf("read data is fail\n");
		return -1;
	}
	i = 0;
#if 1
	for(y=0; y<480; y++)
	{
		for(x=0; x<600 ; x++)
		{
			long location = (y * varinfo.xres * 2) + x *2;
			new_rgb.RGB_h = buff[i];
			new_rgb.RGB_l = buff[i+1];
			rgb = (new_rgb.RGB_l << 8) | new_rgb.RGB_h;

			*((unsigned short *)(virtual_addr + location)) = rgb;
			i = i+2;
		}
	}
#endif 
}

int stop_capturing(int fd)
{
	//7.关闭摄像头
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(fd,VIDIOC_STREAMOFF,&type) == -1){
		return -1;
	}

}
int uninit_camera(int fd)
{
	int i;
	for(i=0; i<4; i++)
	{
		munmap(buffer[i].start, buffer[i].length);
	}

	free(buffer);

	close(fd);

}

int main(int argc, const char *argv[])
{
	int fd,ret;
	fd = open_camera_driver();
	if(fd == -1){
		perror("open /dev/video0 is fail");
		return -1;
	}
	open_lcd_driver();

	ret = init_camera_attitude(fd);
	if(ret == -1){
		perror("init camera is fail");
		return -1;
	}
	init_lcd_attitude();

	start_capturing(fd);

	while(1){
		read_data(fd);
	}

	stop_capturing(fd);

	uninit_camera(fd);

	return 0;
}
