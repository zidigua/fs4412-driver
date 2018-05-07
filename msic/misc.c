#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>


ssize_t misc_read(struct file *filp, char __user *usr, size_t size, loff_t *n)
{
	printk(KERN_INFO"%s %d \n", __func__, __LINE__);
	return 0;
}

ssize_t misc_write(struct file *filp, const char __user *usr, size_t size, loff_t *n)
{
	printk(KERN_INFO"%s %d \n", __func__, __LINE__);

	return 0;
}

int misc_open(struct inode *inod, struct file *filp)
{
	printk(KERN_INFO"%s %d \n", __func__, __LINE__);

	return 0;
}

static struct file_operations fops  = {
	.open = misc_open,
	.read = misc_read,
	.write = misc_write,
};

static struct miscdevice miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "misc",
	.fops = &fops,
};



static int misc_init(void)
{
	return  misc_register(&miscdev);

}


static void misc_exit(void)
{
	misc_deregister(&miscdev);
	return;
}


module_init(misc_init);
module_exit(misc_exit);

MODULE_LICENSE("GPL");
