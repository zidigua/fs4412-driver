#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define SMPLRT_DIV 0x19 //采样频率寄存器，寄存器集合里的数据根据采样频率更新,典型值0x07(125kHz)
#define CONFIG 0x1A //配置寄存器,典型值0x06(5Hz)
#define GYRO_CONFIG		0x1B//陀螺仪配置-27,可以配置自检和满量程范围
									//典型值：0x18(不自检，2000deg/s)
#define ACCEL_CONFIG		0x1C	//加速度配置-28 可以配置自检和满量程范围及高通滤波频率
										//典型值：0x01(不自检，2G，5Hz)
#define ACCEL_XOUT_H	0x3B //59-65,加速度计测量值 XOUT_H
#define ACCEL_XOUT_L	0x3C  // XOUT_L
#define ACCEL_YOUT_H	0x3D  //YOUT_H
#define ACCEL_YOUT_L	0x3E  //YOUT_L
#define ACCEL_ZOUT_H	0x3F  //ZOUT_H
#define ACCEL_ZOUT_L	0x40 //ZOUT_L---64
#define TEMP_OUT_H		0x41 //温度测量值--65
#define TEMP_OUT_L		0x42
#define GYRO_XOUT_H		0x43 //陀螺仪值--67，采样频率（由寄存器 25 定义）写入到这些寄存器
#define GYRO_XOUT_L		0x44
#define GYRO_YOUT_H		0x45
#define GYRO_YOUT_L		0x46
#define GYRO_ZOUT_H		0x47
#define GYRO_ZOUT_L		0x48 //陀螺仪值--72
#define PWR_MGMT_1		0x6B //电源管理 典型值：0x00(正常启用)

#define INT_ENABLE		0x38 //
#define INT_PIN_CFG		0x37

#define IS_TIMER 0

//自定义设备对象
struct mpu_sensor {
    int dev_major;
    struct i2c_client *client;  //记录probe中的client
};

struct mpu_sensor *mpu_dev;
#if IS_TIMER
static struct timer_list my_timer;
#endif
struct input_dev *inputdev;

static struct work_struct mywork; 
static struct i2c_client *g_client = NULL;

int mpu6050_write_bytes(struct i2c_client *client,char *buf,int count)
{
    int ret;
    struct i2c_adapter * adapter=client->adapter;
    struct i2c_msg  msg;

    msg.addr=client->addr;
    msg.flags=0;
    msg.len=count;
    msg.buf=buf;
    
    ret=i2c_transfer(adapter,&msg,1);

    return ret==1?count:ret;
}

int mpu6050_read_bytes(struct i2c_client *client,char *buf,int count)
{
    int ret;
    struct i2c_adapter * adapter=client->adapter;
    struct i2c_msg  msg;

    msg.addr=client->addr;
    msg.flags=I2C_M_RD;
    msg.len=count;
    msg.buf=buf;
    
    ret=i2c_transfer(adapter,&msg,1);

    return ret==1?count:ret;
}

//读取某个特定寄存器的地址
int mpu6050_read_reg_byte(struct i2c_client *client,char reg)
{
    int ret;
    char rxbuf[1];
    struct i2c_adapter *adapter=client->adapter;
    struct i2c_msg msg[2];

    //先写寄存器地址，在读取值
    msg[0].addr=client->addr;
    msg[0].flags=0;
    msg[0].len=1;
    msg[0].buf=&reg;

    msg[1].addr=client->addr;
    msg[1].flags=I2C_M_RD;
    msg[1].len=1;
    msg[1].buf=rxbuf;

    ret=i2c_transfer(adapter,msg,2);

    if(ret<0){
        printk("i2c_transfer read error\n");
        return ret;
    }
       
    return rxbuf[0];
}

static int simple_input_init(void)
{
	int ret;
	
	inputdev = input_allocate_device();
	if(inputdev == NULL){
		printk(KERN_ERR "input_allocatre_device error\n");
		return -ENOMEM;
	}

	//添加设备信息 /sys/class/input/event*/device/
	inputdev->name="simple input key";
	inputdev->phys="key/input/input0";
	inputdev->uniq="simple key0 for 4412";
	inputdev->id.bustype=BUS_HOST;
	inputdev->id.vendor=0x1234;
	inputdev->id.product=0x8888;
	inputdev->id.version=0x0001;

	//当前设备能支持绝对坐标
	__set_bit(EV_ABS, inputdev->evbit);

	input_set_abs_params(inputdev, ABS_RX, 0, 23040, 0, 0);
	input_set_abs_params(inputdev, ABS_RY, -11520, 11520, 0, 0);
	input_set_abs_params(inputdev, ABS_RZ, -5760, 5760, 0, 0);

	ret = input_register_device(inputdev);
	if(ret != 0) {
		printk(KERN_ERR "input_allocatre_device error\n");
		goto err_0;
	}
	
	return 0;

err_0:
	input_free_device(inputdev);
	return ret;
}

#if IS_TIMER
static void on_time_out(unsigned long arg)
{
	schedule_work(&mywork);	
	return ;
}
#else

static irqreturn_t mpu6050_irq(int irq, void *dev_instance)
{
	schedule_work(&mywork);
	return IRQ_HANDLED;
}
#endif

void work_mpu6050_half(struct work_struct *work)
{

	short x = mpu6050_read_reg_byte(mpu_dev->client, ACCEL_XOUT_H) << 8 | \
			                mpu6050_read_reg_byte(mpu_dev->client, ACCEL_XOUT_L);
	short y = mpu6050_read_reg_byte(mpu_dev->client, ACCEL_YOUT_H) << 8 | \
			                mpu6050_read_reg_byte(mpu_dev->client, ACCEL_YOUT_L);
	short z = mpu6050_read_reg_byte(mpu_dev->client, ACCEL_ZOUT_H) << 8 | \
			                mpu6050_read_reg_byte(mpu_dev->client, ACCEL_ZOUT_L);
	
	input_report_abs(inputdev, ABS_RX, x);
	input_report_abs(inputdev, ABS_RY, y);
	input_report_abs(inputdev, ABS_RZ, z);
	input_sync(inputdev);

#if IS_TIMER
	mod_timer(&my_timer, jiffies + 100);
#endif
	
}


int mpu6050_drv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	char buf1[2]={PWR_MGMT_1,0x0};
	char buf2[2]={SMPLRT_DIV,0x07};
	char buf3[2]={CONFIG,0x06};
	char buf4[2]={GYRO_CONFIG,0x18};
	char buf5[2]={ACCEL_CONFIG,0x01};
	char buf6[2]={INT_ENABLE,0x01};
	char buf7[2]={INT_PIN_CFG,0x9c};

	int retval;

	//申请设备号，创建设备文件初始化i2c从设备
	mpu_dev = kzalloc(sizeof(struct mpu_sensor),GFP_KERNEL);
	mpu_dev->client = client;
	g_client = client;

	mpu6050_write_bytes(mpu_dev->client,buf1,2);
	mpu6050_write_bytes(mpu_dev->client,buf2,2);
	mpu6050_write_bytes(mpu_dev->client,buf3,2);
	mpu6050_write_bytes(mpu_dev->client,buf4,2);
	mpu6050_write_bytes(mpu_dev->client,buf5,2);
	mpu6050_write_bytes(mpu_dev->client,buf6,2);
	mpu6050_write_bytes(mpu_dev->client,buf7,2);

	simple_input_init();
#if !IS_TIMER
	retval = request_irq(client->irq, mpu6050_irq,
			    IRQF_TRIGGER_RISING,
			    client->dev.driver->name, NULL);
	
	printk("---%d ----\n", client->irq);
	if(retval) {
		return retval;
	}

#else
	setup_timer(&my_timer, on_time_out, (unsigned long)client);
	my_timer.expires = jiffies + 100;
	add_timer(&my_timer);
#endif

	INIT_WORK(&mywork, work_mpu6050_half);

	return 0;
}

int mpu6050_drv_remove(struct i2c_client *client)
{
#if !IS_TIMER
	free_irq(client->irq, NULL);
#endif
	kfree(mpu_dev);
	input_unregister_device(inputdev);
	input_free_device(inputdev);
	return 0;
}

const struct of_device_id of_mpu6050_id[]={
    {
        .compatible="invensense,mpu6050",
    },
    {/*northing to be done*/},
};

const struct i2c_device_id mpu_id_table[]={
    {"mpu6050_drv",0x1100},
    {/*northing to be done*/},  
};

struct i2c_driver mpu6050_drv={
    .probe=mpu6050_drv_probe,
    .remove=mpu6050_drv_remove,
    .driver={
        .name="mpu6050_drv",
        .of_match_table=of_match_ptr(of_mpu6050_id),
    },
    .id_table=mpu_id_table, //用于费设备树情况下匹配
};

static int __init mpu6050_drv_init(void)
{
    return i2c_add_driver(&mpu6050_drv);;
}

static void __exit mpu6050_drv_exit(void)
{
    i2c_del_driver(&mpu6050_drv);
}

module_init(mpu6050_drv_init);
module_exit(mpu6050_drv_exit);
MODULE_LICENSE("GPL");

