#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include "key.h"


struct file_operations fops = {
	.open = key_open,
	.release = key_release,
	.read = key_read,
	.write = key_write,
	.fasync = key_fasync,
	.poll = key_poll,
	.unlocked_ioctl = key_ioctl,
	.mmap = key_mmap,
};


struct of_device_id my_key_table[] = {
	{ .compatible = "key2", },
	{ .compatible = "key3", },
	{ .compatible = "key4", },
	{}
};


struct platform_driver key_driver = {
	.probe = key_probe,
	.remove = key_remove,
	.driver = {
		.name = "my_key_group",
		.of_match_table = of_match_ptr(my_key_table)
	}
};

char* get_full_path(struct file* file)
{
	char *path=NULL, *start=NULL;

	path = kzalloc(PATH_MAX,GFP_KERNEL);
	if(!path) {
		goto err;
	}

	start = d_path(&file->f_path, path, PATH_MAX);

	kfree(path);
	return start;
err:
	return start;
}

int detection_fn (struct device *dev, void *arg)
{
	struct file *filp = (struct file *)arg;

	if (!strcmp(dev->of_node->name, &get_full_path(filp)[5])) {
		filp->private_data = dev_get_drvdata(dev);
		return 1;
	}

	return 0;
}


static int key_open (struct inode *inode, struct file *filp)
{
	int err = 0;
	struct my_key *pkey = NULL;

	filp->private_data = (void *)MAJOR(inode->i_rdev);

	err = driver_for_each_device(&key_driver.driver, NULL, filp, detection_fn);
	if (!err)
		return -EBUSY;
	pkey = (struct my_key *)filp->private_data;

	atomic_set(&pkey->available, 0);

	return 0;
}

ssize_t key_read (struct file *filp, char __user *p, size_t n, loff_t *off)
{
	struct my_key *pkey = (struct my_key *)filp->private_data;

	wait_event_interruptible(pkey->convert_queue, atomic_dec_and_test(&pkey->available));

	printk(" key_read  %d  \n", MAJOR(pkey->dev_no));

	put_user(MAJOR(pkey->dev_no), (int __user *)p);

	return n;
}

static ssize_t key_write (struct file *filp, const char __user *p, size_t n, loff_t *off)
{

	return 0;
}

static long key_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

	return 0;
}

static int key_release (struct inode *inode, struct file *filp)
{

	return 0;
}

static int key_fasync (int fd, struct file *filp, int mode)
{
	struct my_key *pkey = (struct my_key *)filp->private_data;

	return fasync_helper(fd, filp, mode, &pkey->fasyncqueue);
}

static unsigned int key_poll(struct file *filp, struct poll_table_struct *wait)
{
	int mask = 0;
	struct my_key *pkey = (struct my_key *)filp->private_data;

	poll_wait(filp, &pkey->convert_queue,  wait);

	if (atomic_dec_and_test(&pkey->available)) {
		mask |= POLLIN | POLLRDNORM;  /* readable */
		atomic_set(&pkey->available, 1);
	}

	return mask;
}

static int key_mmap (struct file *filp, struct vm_area_struct *vma)
{

	return 0;
}

static void my_key_down(unsigned long arg)
{
	struct my_key *pkey = (struct my_key *)arg;

	printk(" my_key_down --- %#x  \n",  MAJOR(pkey->cdev.dev));
	kill_fasync(&pkey->fasyncqueue, SIGIO, POLL_IN);
	atomic_set(&pkey->available, 1);
	wake_up_interruptible(&pkey->convert_queue);

	return ;
}

irqreturn_t key_interrupte (int n, void *arg)
{
	struct my_key *key = (struct my_key *)arg;

//	printk("key_complete MAJOR %d MINOR %d  \n", MAJOR(key->cdev.dev), MINOR(key->cdev.dev));
	mod_timer(&key->my_timer, jiffies + 50);

	return IRQ_HANDLED;
}


static struct my_key *alloc_key (const char *name)
{
	struct my_key *pkey = NULL;
	int ret = 0;

	pkey = kzalloc(sizeof(struct my_key), GFP_KERNEL);
	if (!pkey)
		return NULL;

	ret =  alloc_chrdev_region(&pkey->dev_no, 0, 1, "key_no");
	if (ret < 0) {
		printk(" alloc_chrdev_region fail\n");
		goto err_mallo;
	}
	cdev_init(&pkey->cdev, &fops);
	pkey->cdev.owner = THIS_MODULE;
	ret = cdev_add(&pkey->cdev, pkey->dev_no, 1);
	if (ret < 0) {
		printk("cdev_add fail\n");
		goto err_add;
	}

	pkey->cls = class_create(THIS_MODULE, name);
    if (IS_ERR(pkey->cls))
		goto err_class;

    pkey->test_device = device_create(pkey->cls, NULL, pkey->dev_no, NULL, name);  //mknod /dev/hello
    if(IS_ERR(pkey->test_device))
       goto err_device;

	return pkey;

err_device:
	class_destroy(pkey->cls);
err_class:
	cdev_del(&pkey->cdev);
err_add:
	unregister_chrdev_region (pkey->dev_no, ARRAY_SIZE(my_key_table)-1);
err_mallo:
	kfree(pkey);
	return NULL;
}

int init_key (struct platform_device *pdev, struct my_key *pkey)
{
	int retval = 0;

	pkey->key_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	if (!pkey->key_irq)
		goto err_get_res;

	retval = request_irq(pkey->key_irq->start, key_interrupte,
			(pkey->key_irq->flags& IRQF_TRIGGER_MASK) | IRQF_DISABLED, "key down",
				pkey);
	if (retval)
		goto err_get_res;

	setup_timer(&pkey->my_timer, my_key_down, (unsigned long)pkey);
	pkey->my_timer.expires = jiffies + 50;
	add_timer(&pkey->my_timer);

	printk("------init_key----%d -----\n", pkey->key_irq->start);

	return 0;

err_get_res:
	retval = -EINVAL;
	kfree(pkey);
	return retval;
}

int key_probe (struct platform_device *pdev)
{
	int retval = 0;
	struct my_key *pkey;

	printk("key_probe !\n");
	pkey = alloc_key(pdev->dev.of_node->name);
	if(NULL == pkey)
		return -ENOMEM;

	retval = init_key(pdev, pkey);
	if(retval)
		goto err_init;
	retval = dev_set_drvdata(&pdev->dev, pkey);
	if(retval)
		goto err_init;

	mutex_init(&pkey->lock);
	init_waitqueue_head(&pkey->convert_queue);
	atomic_set(&pkey->available, 0);

	kill_fasync(&pkey->fasyncqueue, SIGIO, POLL_IN);


	return retval;

err_init:
	kfree(pkey);
	return -ENOMEM;
}



int key_remove (struct platform_device *pdev)
{
	struct my_key *pkey;

	pkey = (struct my_key *)dev_get_drvdata(&pdev->dev);


	free_irq(pkey->key_irq->start, pkey);

	device_del(pkey->test_device);

	class_destroy(pkey->cls);

	cdev_del(&pkey->cdev);
	unregister_chrdev_region(pkey->dev_no, 1);

	kfree(pkey);

	printk("key_remove released \n");
	return 0;
}


static __init int my_key_init (void)
{
	return platform_driver_register(&key_driver);
}

static __exit void my_key_exit (void)
{
	platform_driver_unregister(&key_driver);
}


module_init(my_key_init);
module_exit(my_key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxx");
MODULE_DESCRIPTION("xxxx");
