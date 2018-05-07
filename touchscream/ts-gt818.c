#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/input/mt.h>
unsigned int *GPX0CON;
unsigned char ts_cfg_info[106] = {
	0x12,0x10,0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x00,0xE0,0x00,0xD0,0x00,
	0xC0,0x00,0xB0,0x00,0xA0,0x00,0x90,0x00,0x80,0x00,0x70,0x00,0x60,0x00,
	0x50,0x00,0x40,0x00,0x30,0x00,0x20,0x00,0x10,0x00,0x00,0x00,0xF0,0x00,
	0x1B,0x03,0x00,0x00,0x00,0x16,0x16,0x16,0x0F,0x0F,0x0A,0x45,0x32,0x05,
	0x03,0x00,0x05,0x00,0x04,0x58,0x02,0x00,0x00,0x34,0x2C,0x37,0x2E,0x00,
	0x00,0x14,0x14,0x01,0x0A,0x00,0x00,0x00,0x00,0x00,0x14,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x1F,0x18,
	0x1A,0x00,0x00,0x00,0x00,0x00,0x00,0x01
};
struct ts{
	struct i2c_client *client;
	struct input_dev *dev;
	int base_addr;
	int irq_num;
	struct work_struct work;
	spinlock_t lock;
	int touch_num;
	int data_base_addr;
	int data[40];
};
struct ts *ts;
int ts_write(int reg,char val)
{
	int ret;
	char reg_l = (reg & 0xff);
	char reg_h = ((reg>>8)& 0xff);
	char buf[3] = {reg_h,reg_l,val};
	struct i2c_msg write_reg[] = {
		[0] = {
				.addr = ts->client->addr,
				.flags = 0,
				.len = 3,
				.buf = buf,
		},
	};
	ret = i2c_transfer(ts->client->adapter,write_reg,sizeof(write_reg)/sizeof(write_reg[0]));
	if(ret != 1){
		printk("i2c_transfer is fail.\n");
		return -EAGAIN;
	}
	return 0;
}
int ts_read(int reg)
{
	int ret;
	char val;
	char reg_l = (reg & 0xff);
	char reg_h = ((reg>>8)& 0xff);
	char buf[2] = {reg_h,reg_l};
	struct i2c_msg read_reg[] = {
		[0] = {
				.addr = ts->client->addr,
				.flags = 0,
				.len = 2,
				.buf = buf,
		},
		[1] = {
		.addr = ts->client->addr,
		.flags = I2C_M_RD,
		.len = 1,
		.buf = &val,
		},
	};
	ret = i2c_transfer(ts->client->adapter,read_reg,sizeof(read_reg)/sizeof(read_reg[0]));
	if(ret != 2){
		printk("i2c_transfer is fail.\n");
		return -EAGAIN;
	}
	return val;
}
int ts_write_cmd(int reg)
{
	int ret;
	char reg_l = (reg & 0xff);
	char reg_h = ((reg>>8)& 0xff);
	char buf[2] = {reg_h,reg_l};
	struct i2c_msg write_reg[] = {
		[0] = {
				.addr = ts->client->addr,
				.flags = 0,
				.len = 2,
				.buf = buf,
		},
	};
	ret = i2c_transfer(ts->client->adapter,write_reg,sizeof(write_reg)/sizeof(write_reg[0]));
	if(ret != 1){
		printk("i2c_transfer is fail.\n");
		return -EAGAIN;
	}
	return 0;
}
int ts_write_cmd_end(void)
{
	int tryagin = 0;
	int ret;
	while(tryagin++ < 5){
		 ret = ts_write_cmd(0x8000);
		 if(ret == 0){
			return 0;
		 }else{
		 	printk("retrying...........\n");
			mdelay(10);
		 }
	}
	return -EAGAIN;
}
void ts_interrupt_disable(void)
{
	unsigned long flag;
	spin_lock_irqsave(&ts->lock,flag);
	disable_irq_nosync(ts->client->irq);
	spin_unlock_irqrestore(&ts->lock,flag);
}
void ts_interrupt_enable(void)
{
	unsigned long flag;
	spin_lock_irqsave(&ts->lock,flag);
	enable_irq(ts->client->irq);
	spin_unlock_irqrestore(&ts->lock,flag);
}
static irqreturn_t ts_interrupt(int irq, void *dev_id)
{
	ts_interrupt_disable();
	schedule_work(&ts->work);
	return IRQ_HANDLED;
}
void ts_work(struct work_struct *work)
{
	int i,ret;
	ts->data_base_addr = 0x722;
	ret = ts_read(0x712);
	ts_write_cmd_end();
	ret = (ret & 0xf);
	ts->touch_num = ret;
	for(i=0; i<(ts->touch_num*8); i++){
		ts->data[i] = ts_read(ts->data_base_addr);
		ts_write_cmd_end();
		ts->data_base_addr +=1;
	}
	for(i=0; i<ts->touch_num; i++){
		printk("touch:(%d,%d)\n",(ts->data[i*8+2]<<8|ts->data[i*8+1]),(ts->data[i*8+4]<<8|ts->data[i*8+3]));
	}
	for(i=0; i<ts->touch_num; i++){
		input_mt_slot(ts->dev, ts->data[i*8]);
		input_report_abs(ts->dev,ABS_MT_TRACKING_ID,ts->data[i*8]);
		input_report_abs(ts->dev,ABS_MT_POSITION_X,(ts->data[i*8+2]<<8|ts->data[i*8+1]));
		input_report_abs(ts->dev,ABS_MT_POSITION_Y,(ts->data[i*8+4]<<8|ts->data[i*8+3]));
	}
	input_sync(ts->dev);
	ts_interrupt_enable();
}
int ts_init_input(void)
{
	int ret;
	//分配结构体
	ts->dev = input_allocate_device();
	if(ts->dev == NULL){
		printk("alloc input memory is fail.\n");
		return -ENOMEM;
	}
	//设置事件类型
		set_bit(EV_ABS,ts->dev->evbit);
		set_bit(EV_KEY,ts->dev->evbit);
	//设置上报那个事件
		input_mt_init_slots(ts->dev,5,0);
		input_set_abs_params(ts->dev,ABS_MT_SLOT,0,5,0,0);
		input_set_abs_params(ts->dev,ABS_MT_POSITION_X,0,1024,0,0);
		input_set_abs_params(ts->dev,ABS_MT_POSITION_Y,0,600,0,0);
		input_set_abs_params(ts->dev,ABS_MT_PRESSURE,0,1024,0,0);
		input_set_abs_params(ts->dev,ABS_MT_TRACKING_ID,0,600,0,0);
	//注册
	ret = input_register_device(ts->dev);
	if(ret){
		printk("input_register_device is fail.\n");
		return -EAGAIN;
	}
	return 0;
}
int gt818_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i,ret;
	struct device_node *node;
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	mdelay(30);
	//复位
	GPX0CON = ioremap(0x11000c00,4);
	*GPX0CON &= ~(0xf<<12);
	*GPX0CON |=  (1<<12);
	*(GPX0CON+1) |= (1<<3); 
	//分配结构体
	ts = kzalloc(sizeof(*ts),GFP_KERNEL);
	if(ts == NULL){
		printk("alloc memory is fail.\n");
		return -ENOMEM;
	}
	ts->client = client;
	ret = ts_write_cmd_end();
	if(ret == 0){
		printk("ts_write_cmd_end is success.\n");
	}else{
		printk("ts_write_cmd_end is fail.\n");
	}
	//初始化
	ts->base_addr = 0x6a1;
	for(i=0; i<106; i++){
		ts->base_addr += 1;
		ret = ts_write(ts->base_addr,ts_cfg_info[i]);
		if(ret){
			printk("write reg is fail.\n");
			return -EAGAIN;
		}
		ts_write_cmd_end();
	}
	spin_lock_init(&ts->lock);
	INIT_WORK(&ts->work,ts_work);
	ts_init_input();
	//注册中断
	ret = request_irq(ts->client->irq,ts_interrupt,IRQF_TRIGGER_RISING,"ts-interrupt",NULL);
	if(ret){
		printk("request_irq is fail.\n");
		return -EAGAIN;
	}
	return 0;
}
int gt818_remove(struct i2c_client *client)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}
const struct of_device_id ts_of_match[] = {
	{ .compatible = "gt8818", },
	{},
};
MODULE_DEVICE_TABLE(of, ts_of_match);
static const struct i2c_device_id ts_i2c_id[] = {
	{ "ts_gt818", 0x5d },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ts_i2c_id);
static struct i2c_driver ts_driver ={
	.driver = {
		.name	= "ts_gt818",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(ts_of_match),
	},
	.probe		= gt818_probe,
	.remove		= gt818_remove,
	.id_table	= ts_i2c_id,
}; 
static int __init ts_gt818_init(void)
{
	i2c_add_driver(&ts_driver);
	return 0;
}
static void __exit ts_gt818_exit(void)
{
	i2c_del_driver(&ts_driver);
}
module_init(ts_gt818_init);
module_exit(ts_gt818_exit);
MODULE_LICENSE("GPL");
