#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include "adc.h"


struct file_operations fops = {
	.open = adc_open,
	.release = adc_release,
	.read = adc_read,
	.write = adc_write,
	.unlocked_ioctl = adc_ioctl,
	.mmap = adc_mmap,
};


struct of_device_id adc_table[] = {
	{ .compatible = "adc", },
	{ }
};


struct platform_driver adc_driver = {
	.probe = adc_probe,
	.remove = adc_remove,
	.driver = {
		.name = "xxxxxxx",
		.of_match_table = of_match_ptr(adc_table)
	}
};


int detection_fn (struct device *dev, void *arg)
{
	struct file *filp = (struct file *)arg;
	const char *name = NULL;

	of_property_read_string(dev->of_node, "compatible", &name);

	if (!strcmp(name, adc_driver.driver.of_match_table[0].compatible)) {
		filp->private_data = dev_get_drvdata(dev);
		return 0;
	}
	return -1;
}

static int adc_open (struct inode *inode, struct file *filp)
{
	int err = 0;
	err = driver_for_each_device(&adc_driver.driver, NULL, filp, detection_fn);
	if (err)
		return -EBUSY;

	return 0;
}

ssize_t adc_read (struct file *filp, char __user *p, size_t n, loff_t *off)
{
	unsigned int num = 0, ret = 0;
	struct my_adc *padc = (struct my_adc *)filp->private_data;


	mutex_lock(&padc->lock);
	iowrite32((ioread32(padc->ADCCON) | 1), padc->ADCCON);
	wait_event_interruptible(padc->convert_queue, ioread32(padc->ADCCON) & (1 << 15));
	mutex_unlock(&padc->lock);
	num = ioread32(padc->ADCDAT) & 0xFFF;

	ret = put_user(num, (int __user *)p);

	return 10;

}

static ssize_t adc_write (struct file *filp, const char __user *p, size_t n, loff_t *off)
{

	return 0;
}

static long adc_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

	return 0;
}

static int adc_release (struct inode *inode, struct file *filp)
{

	return 0;
}

static int adc_mmap (struct file *filp, struct vm_area_struct *vma)
{

	return 0;
}


irqreturn_t adc_complete (int n, void *arg)
{
	struct my_adc *adc = (struct my_adc *)arg;


	//printk("ADCDAT	: %#x \n", ioread32(adc->ADCDAT) & 0xFFF);
	iowrite32((ioread32(adc->CLRINTADC) | 1), adc->CLRINTADC);

	wake_up_interruptible(&adc->convert_queue);


	return IRQ_HANDLED;
}


static struct my_adc *alloc_adc (void)
{
	struct my_adc *padc = NULL;
	int ret = 0;

	padc = kzalloc(sizeof(struct my_adc), GFP_KERNEL);
	if (!padc)
		return NULL;

	ret =  alloc_chrdev_region(&padc->dev_no, 0, 1, "adc_no");
	if (ret < 0) {
		printk(" alloc_chrdev_region fail\n");
		goto err_mallo;
	}
	cdev_init(&padc->cdev, &fops);
	padc->cdev.owner = THIS_MODULE;
	ret = cdev_add(&padc->cdev, padc->dev_no, 1);
	if (ret < 0) {
		printk("cdev_add fail\n");
		goto err_add;
	}

	padc->cls = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(padc->cls))
		goto err_class;

    padc->test_device = device_create(padc->cls, NULL, padc->dev_no, NULL, "hello");  //mknod /dev/hello
    if(IS_ERR(padc->test_device))
       goto err_device;


	return padc;

err_device:
	class_destroy(padc->cls);
err_class:
	cdev_del(&padc->cdev);
err_add:
	unregister_chrdev_region (padc->dev_no, 1);
err_mallo:
	kfree(padc);
	return NULL;
}

int init_adc (struct platform_device *pdev, struct my_adc *padc)
{
	int retval = 0, i = 0;

	padc->irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!padc->irq_res) {
		printk("get irq error");
		retval = -EINVAL;
		goto err_get_res;
	}

	for (i = 0; i < 5; i++) {
		padc->adc_reg[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if(!padc->adc_reg[i]) {
			printk("get adc reg error");
			retval = -EINVAL;
			goto err_get_res;
		}
	}

	padc->ADCCON =	ioremap(padc->adc_reg[0]->start, 4) ;
	if (!padc->ADCCON)
		goto err_con;
	padc->ADCDLY = ioremap(padc->adc_reg[1]->start, 4) ;
	if (!padc->ADCDLY)
		goto err_dly;
	padc->ADCDAT = ioremap(padc->adc_reg[2]->start, 4) ;
	if (!padc->ADCDAT)
		goto err_dat;
	padc->CLRINTADC = ioremap(padc->adc_reg[3]->start, 4) ;
	if (!padc->CLRINTADC)
		goto err_clrint;
	padc->ADCMUX = ioremap(padc->adc_reg[4]->start, 4) ;
	if(!padc->ADCMUX)
		goto err_mux;

	iowrite32((ioread32(padc->ADCCON) | (1 << 16)), padc->ADCCON);
	iowrite32((ioread32(padc->ADCCON) | (1 << 14)), padc->ADCCON);
	iowrite32((ioread32(padc->ADCCON) & (~(0xFF << 6))) | (19 << 6), padc->ADCCON);
	iowrite32((ioread32(padc->ADCCON) & (~(1 << 2))), padc->ADCCON);
	iowrite32((ioread32(padc->ADCCON) & (~(1 << 1))), padc->ADCCON);
	iowrite32( 0x3, padc->ADCMUX);
//	iowrite32((ioread32(padc->ADCCON) | 1), padc->ADCCON);

	retval = request_irq(padc->irq_res->start, adc_complete,
			(padc->irq_res->flags& IRQF_TRIGGER_MASK) | IRQF_DISABLED, "adc convert",
				padc);
	if (retval)
		goto err_req_irq;


	return 0;

err_req_irq:
	iounmap(padc->ADCMUX);
err_mux:
	iounmap(padc->CLRINTADC);
err_clrint:
	iounmap(padc->ADCDAT);
err_dat:
	iounmap(padc->ADCDLY);
err_dly:
	iounmap(padc->ADCCON);
err_con:
	retval = -ENOMEM;
err_get_res:

	return retval;

}

int adc_probe (struct platform_device *pdev)
{
	int retval = 0;
	struct my_adc *padc;
	printk("adc_probe !\n");
	padc = alloc_adc();
	if(NULL == padc)
		return -ENOMEM;


	retval = init_adc(pdev, padc);
	if(retval)
		goto err_init;

	retval = dev_set_drvdata(&pdev->dev, padc);
	if(retval)
		goto err_init;

	mutex_init(&padc->lock);
	init_waitqueue_head(&padc->convert_queue);


	return retval;

err_init:
	kfree(padc);
	return -ENOMEM;
}



int adc_remove (struct platform_device *pdev)
{
	struct my_adc *padc;

	padc = (struct my_adc *)dev_get_drvdata(&pdev->dev);

	free_irq(padc->irq_res->start, padc);

	iounmap(padc->ADCCON);
	iounmap(padc->ADCDLY);
	iounmap(padc->ADCDAT);
	iounmap(padc->CLRINTADC);
	iounmap(padc->ADCMUX);

	device_del(padc->test_device);
	class_destroy(padc->cls);

	cdev_del(&padc->cdev);
	unregister_chrdev_region(padc->dev_no, 1);

	kfree(padc);

	printk("adc_remove released\n");
	return 0;
}





static __init int adc_init (void)
{
	return platform_driver_register(&adc_driver);
}

static __exit void adc_exit (void)
{
	platform_driver_unregister(&adc_driver);
}


module_init(adc_init);
module_exit(adc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("xxxx");
