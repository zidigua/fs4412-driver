#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/highmem.h> 
#include <asm/kmap_types.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("xxxx");
MODULE_DESCRIPTION("xxxx");


struct my_led{
	dev_t dev_no;
	struct cdev *cdev;
	struct file_operations *ops;
	unsigned int *reg_con;
	unsigned char *reg_dat;
	int flag;
};

struct my_led *led = NULL;

static int led_open (struct inode *inode, struct file *filp)
{
	filp->private_data = led;


	return 0;
}

ssize_t led_read (struct file *filp, char __user *p, size_t n, loff_t *off)
{
	struct my_led *led = (struct my_led *)(filp->private_data);
	if(led != NULL) {
		printk("led->flag %d \n", led->flag);
		copy_to_user(p, &led->flag, 5);
	}
	return 0;
}

static ssize_t led_write (struct file *filp, const char __user *p, size_t n, loff_t *off)
{
	struct my_led *led = (struct my_led *)(filp->private_data);
	copy_from_user(&led->flag, p, 4);

	printk("led->flag %#x \n", led->flag);

	return n;
}

#define ON 0
#define OFF 1

static long led_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) 
{
	struct my_led *led = (struct my_led *)(filp->private_data);
	printk("---inode--%p------file---%p---<%d>---\n", filp->f_inode, filp, __LINE__);   
	switch(cmd) {
	case ON:
		 writeb(readb(led->reg_dat) | (1<<7), led->reg_dat);
		 break;
	case OFF:
		 writeb(readb(led->reg_dat) & (~(1<<7)), led->reg_dat);
		 break;
	}

	return 0;
}

static int led_release (struct inode *inode, struct file *filp)
{
	printk("---inode--%p------file---%p---<%d>---\n", inode, filp, __LINE__);   
	filp->private_data = NULL;
	printk(" led_release  \n");
	return 0;
}

int led_mmap(struct file *filp, struct vm_area_struct *vma) 
{
	return remap_pfn_range(vma, vma->vm_start, 0x11000000 >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);
}
struct file_operations led_ops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.read = led_read,
	.write = led_write,
	.unlocked_ioctl = led_ioctl,
	.mmap = led_mmap,
};

static __init int init_cdev(void) 
{
	int ret = 0;
	led = kmalloc(sizeof(struct my_led), GFP_KERNEL);
	if(!led) {
		printk("malloc error");
		return -ENOMEM;
	}
	ret =  alloc_chrdev_region(&led->dev_no, 0, 1, "hello led");
	if(ret < 0) {
		goto mallo;
	}
	led->cdev = cdev_alloc();
	if(led->cdev < 0) {
		goto alloc;
	}
	cdev_init(led->cdev, &led_ops);
	ret = cdev_add(led->cdev, led->dev_no, 1);
	if(ret < 0) {
		goto add;
	}

	led->reg_con = ioremap(0x11000c40, 4);
	led->reg_dat = ioremap(0x11000c44, 1);


	return 0;
add:
	cdev_del(led->cdev);
	kfree(led->cdev);
alloc:
	unregister_chrdev_region(led->dev_no, 1);
mallo:
	kfree(led);
	return -1;

}

static __exit void exit_cdev(void)
{
	iounmap(led->reg_con);
	iounmap(led->reg_dat);
	cdev_del(led->cdev);
	kfree(led->cdev);
	unregister_chrdev_region(led->dev_no, 1);
	printk("%p \n", led->cdev);
	kfree(led);

	return;
}


module_init(init_cdev);
module_exit(exit_cdev);
