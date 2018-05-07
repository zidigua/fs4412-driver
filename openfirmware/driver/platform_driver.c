#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int my_probe(struct platform_device *dev);
static int my_remove(struct platform_device *dev);
static int led_open (struct inode *inode, struct file *filp);
static long led_ioctl (struct file *filp, unsigned int cmd, unsigned long flag);
static int led_release (struct inode *inode, struct file *filp);

static struct file_operations fops = {
	.open = led_open,
	.unlocked_ioctl = led_ioctl, 
	.release = led_release,
};

struct led_cdev {
	dev_t dev_no;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	unsigned long *reg_con;
}my_led;

#ifndef CONFIG_OF
static struct platform_device_id led_id_table[] = {
	[0] = {
		.name = "plat_led2",
		.driver_data = 1234,
	},
	[1] = {
		.name = "plat_led3",
		.driver_data = 4321,
	},
	{}
};
MODULE_DEVICE_TABLE(platform, led_id_table);
#else
static struct of_device_id of_id_table[] = {
	[0] = {
		.compatible = "ledxxx",
	},
	{}
};
MODULE_DEVICE_TABLE(of, of_id_table);
#endif


static struct platform_driver my_led_driver = {
	.probe = my_probe,
	.remove = my_remove,
#ifndef CONFIG_OF
	.id_table =  of_match_ptr(led_id_table),
#endif
	.driver = {
		.of_match_table = of_match_ptr(of_id_table),
		.name = "plat_led",
	},
};

static int led_open (struct inode *inode, struct file *filp)
{
	printk("led_open led_open\n");
	
	return 0;
}

static long led_ioctl (struct file *filp, unsigned int cmd, unsigned long flag)
{
	printk("led_ioctl led_ioctl\n");

	return 0;	
}

static int led_release (struct inode *inode, struct file *filp) 
{
	printk("led_release led_release\n");

	return 0;
}

#if 0
#define to_device_private_driver(obj)	\
	container_of(obj, struct device_private, knode_driver)

struct device *next_device(struct klist_iter *i)
{
	struct klist_node *n = klist_next(i);
	struct device *dev = NULL;
	struct device_private *dev_prv;

	if (n) {
		dev_prv = to_device_private_driver(n);
		dev = dev_prv->device;
	}
	return dev;
}
#endif

static ssize_t led_show(struct device_driver *driver, char *buf)
{

#if 0
	struct klist_iter i;
	struct device *dev;

	printk("led_show  %p \n", driver);
	klist_iter_init_node(&driver->p->klist_devices, &i, NULL);
	while ((dev = next_device(&i)) ) {
		printk("------%s ------- \n", dev_name(dev));
	}

	klist_iter_exit(&i);
#endif

	if(driver) {
		writeb(readb(my_led.reg_con + 1) | (1 << 7), my_led.reg_con + 1);
	}
	return 0;
}

static ssize_t led_store(struct device_driver *driver, const char *buf, size_t count)
{
#if 0

	struct klist_iter i;
	struct device *dev;

	printk("led_store  %p \n", driver);
	klist_iter_init_node(&driver->p->klist_devices, &i, NULL);
	while ((dev = next_device(&i)) ) {
		printk("------%s ------- \n", dev_name(dev));
	}

	klist_iter_exit(&i);
#endif

	if(driver) {
		writeb(readb(my_led.reg_con + 1) & (~(1 << 7)), my_led.reg_con + 1);
	}
	return 0;
}

static DRIVER_ATTR_RW(led);


static int my_probe(struct platform_device *dev) 
{

	struct resource *res = platform_get_resource(dev, IORESOURCE_MEM, 0);

	int age;
	const char *name;
	of_property_read_u32(dev->dev.of_node, "age", &age);
	of_property_read_string(dev->dev.of_node, "myname", &name);


	printk("my_probe start %#x \n", res->start);	
	printk("my_probe end %#x \n", res->end);
	printk("my_probe age %#x \n", age);	
	printk("my_probe name %s \n", name);	


	my_led.reg_con = ioremap(res->start, 8);

	alloc_chrdev_region(&my_led.dev_no, 0, 1, "led --- platform");

	cdev_init(&my_led.cdev, &fops);
	cdev_add(&my_led.cdev, my_led.dev_no, 1);

	my_led.class = class_create(THIS_MODULE, "myclass");  
    if(IS_ERR(my_led.class)) {
		cdev_del(&my_led.cdev);
		unregister_chrdev_region(my_led.dev_no, 1);
		iounmap(my_led.reg_con);
        return -EBUSY;
    }  
    my_led.device = device_create(my_led.class, NULL, my_led.dev_no, NULL, "hello");  
    if(IS_ERR(my_led.device)) {
        class_destroy(my_led.class);
        cdev_del(&my_led.cdev);
		unregister_chrdev_region(my_led.dev_no, 1);
		iounmap(my_led.reg_con);
        return -EBUSY;
    } 

	return 0;
}

static int my_remove(struct platform_device *dev) 
{
	printk(" my_remove my_remove \n");
	device_del(my_led.device);
	class_destroy(my_led.class);
	cdev_del(&my_led.cdev);
	unregister_chrdev_region(my_led.dev_no, 1);
	iounmap(my_led.reg_con);
	
	return 0;
}


static __init int init_led(void) 
{
	platform_driver_register(&my_led_driver);
	
	if(driver_create_file(&my_led_driver.driver, &driver_attr_led)) {
		platform_driver_unregister(&my_led_driver);
		return -1;
	}
	return 0;
}

static __exit void exit_led(void)
{
	driver_remove_file(&my_led_driver.driver, &driver_attr_led);
	platform_driver_unregister(&my_led_driver);	
}

module_init(init_led);
module_exit(exit_led);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("xxxx");
MODULE_DESCRIPTION("xxxx");
