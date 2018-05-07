#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <stdio.h>

#define DEV_PATH "/dev/i2c-5"

#define SLAVE_ADDR     0X68
#define SMPLRT_DIV     0x19
#define CONFIG         0x1A
#define GYRO_CONFIG    0x1B
#define ACCEL_CONFIG    0x1C
#define ACCEL_XOUT_H    0x3B
#define ACCEL_XOUT_L    0x3C
#define ACCEL_YOUT_H    0x3D
#define ACCEL_YOUT_L    0x3E
#define ACCEL_ZOUT_H    0x3F
#define ACCEL_ZOUT_L    0x40
#define TEMP_OUT_H     0x41
#define TEMP_OUT_L     0x42
#define GYRO_XOUT_H    0x43
#define GYRO_XOUT_L    0x44
#define GYRO_YOUT_H    0x45
#define GYRO_YOUT_L    0x46
#define GYRO_ZOUT_H    0x47
#define GYRO_ZOUT_L    0x48
#define PWR_MGMT_1     0x6B

unsigned char reg[6][2] = {
	{PWR_MGMT_1,0x40},
	{PWR_MGMT_1,0x01},
	{GYRO_CONFIG,0x18},
	{ACCEL_CONFIG,0x01},
	{SMPLRT_DIV,0x07},
	{CONFIG,0x06},
};
struct i2c_rdwr_ioctl_data write_msg;
struct i2c_rdwr_ioctl_data read_msg;

static int write_reg(int fd,int reg,int val)
{
	int ret;
	char wbuf[2] = {reg,val};
	struct i2c_msg msgs[1] = {
		[0] = {
			.addr = SLAVE_ADDR,
			.flags = 0,          
			.len = 2,
			.buf = wbuf,
		}
	};
	write_msg.msgs = msgs;
	write_msg.nmsgs = sizeof(msgs)/sizeof(msgs[0]);
	ret = ioctl(fd,I2C_RDWR,(unsigned long)&write_msg);/*设置从设备寄存器*/
	if(ret < 0){
		perror("ioctl write\n");
		return -EINVAL;
	}
	return 0;
}

static int read_reg(int fd,int reg)
{
	int ret;
	char txbuf[1] = { reg };  
    char rxbuf[1];  
	struct i2c_msg msgs[2] = {
		[0] = {
			.addr = SLAVE_ADDR,
			.flags = 0,         
			.len = 1,
			.buf = txbuf,
		},
		[1] = {
			.addr = SLAVE_ADDR,
			.flags = I2C_M_RD,          
			.len = sizeof(rxbuf),
			.buf = rxbuf,
		},
	};
	read_msg.msgs = msgs;
	read_msg.nmsgs = sizeof(msgs)/sizeof(msgs[0]); 
	ret = ioctl(fd,I2C_RDWR,(unsigned long)&read_msg);/*设置从设备寄存器*/
	if(ret < 0){
		perror("ioctl write\n");
		return -EINVAL;
	}

	return rxbuf[0];
}

int main(int argc, const char *argv[])
{
	int fd;
	int ret,i,j,value;
	fd = open(DEV_PATH,O_RDWR);/*打开 /dev/i2c-5 */
	if(fd < 0){
		perror("open\n");
		return -EINVAL;
	}
	ret = ioctl(fd,I2C_SLAVE,SLAVE_ADDR);/*设置从机地址*/
	if(ret){
		perror("ioctl slave\n");
		return -EINVAL;
	}
	ret = ioctl(fd,I2C_TENBIT,0);
	if(ret){
		perror("ioctl ten bit\n");
		return -EINVAL;
	}
	ioctl(fd,I2C_TIMEOUT,2);
	ioctl(fd,I2C_RETRIES,1);
	sleep(1);

	for(i=0; i<6; i++){
		write_reg(fd,reg[i][0],reg[1][1]);	
	}
	
	while(1){
		value = read_reg(fd,ACCEL_XOUT_H);
		value = value << 8;
		value |= read_reg(fd,ACCEL_XOUT_L);
		printf("accelx:%d \n",value);
		sleep(2);
	}
	close(fd);
	return 0;
}
