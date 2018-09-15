#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/of.h>

#include <linux/of_gpio.h>


#include <linux/delay.h>

static int gpio_no = 0;
void ds18b20_init(void)
{
	int ack = 1;
	gpio_set_value(gpio_no, 0);	//主机拉低总线
	udelay(495);			//延时495us
	gpio_set_value(gpio_no, 1);	//释放总线，同时IO口产生的上升沿能被DS18B20所检测到
	udelay(60);			//延时大于60us，确保接下来DS18B20能发出60~240us的存在脉冲应答
	ack = gpio_get_value(gpio_no);	//在此60~240us之内DQ被DS18B20所占用，若存在，则其会发送一个低电平信号，DQ被DS18B20拉低，则ack为0，反之为1
	udelay(240);			//延时达240us，让DS18B20释放总线
	gpio_set_value(gpio_no, 1);
}

void ds18b20_write(unsigned char byte)
{
	unsigned char i = 0;
	for (i = 0; i < 8; i++) {
		gpio_set_value(gpio_no, 0);		//拉低总线，产生写时隙
		udelay(2);		//简单延时
		gpio_set_value(gpio_no, 1);		//15us之内释放总线
		udelay(2);		//简单延时
		gpio_set_value(gpio_no, byte & 0x01);	//将字节低位写入单总线
		udelay(50);		//在15~60us内等待DS18B20来采集信号
		gpio_set_value(gpio_no, 1);		//释放总线
		byte >>= 1;       	//每次讲要读取的数据位移至最低位
	}	
}

unsigned char ds18b20_read(void)
{
	unsigned char i = 0;
	unsigned char byte = 0;
	for (i = 0; i < 8; i++) {
		gpio_set_value(gpio_no, 0);//产生读时序
		udelay(2);			//简单延时
		gpio_set_value(gpio_no, 1);//释放总线,有从机DS18B20占用
		byte >>= 1;     		//先进行移位
		if(gpio_get_value(gpio_no))
			byte |= 0x80;
		udelay(60);			//延时60us
		gpio_set_value(gpio_no, 1);	//释放总线
		udelay(1);
	}
	return byte;
}

void read_temp(void) 
{
	unsigned char  tmpl = 0, tmph = 0;
	unsigned short tmp = 0;

	ds18b20_init();
	
	ds18b20_write(0xcc);
	ds18b20_write(0x44);
	while(!gpio_get_value(gpio_no));
	ds18b20_init();
	ds18b20_write(0xcc);
	ds18b20_write(0xbe);
	tmpl = ds18b20_read();
	tmph = ds18b20_read();
	ds18b20_read();
	tmp = (tmph << 4) | (tmpl >> 4);
	printk("%#x %#x tmp %d\n", tmph, tmpl, tmp);

}

static int ds18b20_probe(struct platform_device *pdev)
{
	int retval = 0;

	struct device_node *cnp = pdev->dev.of_node;

	gpio_no = of_get_gpio(cnp, 0);
	if (!gpio_is_valid(gpio_no)) {
		printk("gpio_is_valid %s %d\n", __func__, __LINE__);
		return -1;
	}
	retval = devm_gpio_request(&pdev->dev, gpio_no, "ds18b20");
	if (retval < 0) {
		printk("gpio_request %s %d\n", __func__, __LINE__);
		return retval;
	}
	gpio_direction_output(gpio_no, 0);
	
	printk("gpio_is_valid %s %d\n", __func__, __LINE__);	

	while(1) {
		read_temp();
		ssleep(2);
	}

	return 0;
}

static int ds18b20_remove(struct platform_device *pdev)
{
	
	return 0;
}

static const struct of_device_id fs4412_ds18b20_dt_id[] = {
	{ .compatible = "fs4412-ds18b20", },
	{},
};

static struct platform_driver fs4412_ds18b20 = {
	.driver = {
		.name = "fs4412-ds18b20",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(fs4412_ds18b20_dt_id),
	},
	.probe = ds18b20_probe,
	.remove = ds18b20_remove,
};

module_platform_driver(fs4412_ds18b20);
MODULE_LICENSE("GPL");


