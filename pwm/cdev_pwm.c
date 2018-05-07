#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/io.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxxx");
MODULE_DESCRIPTION("xxxxx");

struct pwm_cdev *pwm = NULL;

/* pwm */
struct pwm_s {
	unsigned int TCFG0;
	unsigned int TCFG1;
	unsigned int TCON;
	unsigned int TCNTB0;
	unsigned int TCMPB0;
	unsigned int TCNTO0;
	unsigned int TCNTB1;
	unsigned int TCMPB1;
	unsigned int TCNTO1;
	unsigned int TCNTB2;
	unsigned int TCMPB2;
	unsigned int TCNTO2;
	unsigned int TCNTB3;
	unsigned int TCMPB3;
	unsigned int TCNTO3;
	unsigned int TCNTB4;
	unsigned int TCNTO4;
	unsigned int TINT_CSTAT;
};

struct pwm_cdev {
	dev_t dev_no;
	struct cdev *dev;
	unsigned char *GPD0_0;
	struct pwm_s *PWM;    /*0x139D0000*/

	int flag;
};

static int pwm_open(struct inode *i, struct file *f)
{

	f->private_data = pwm;
	
	writeb((readb(pwm->GPD0_0) & (~(0xF)) )| (1 << 1), pwm->GPD0_0);
	
	writeb((readb(&pwm->PWM->TCFG0) & (~(0xFF))) | 99, &pwm->PWM->TCFG0);
	
	writeb((readb(&pwm->PWM->TCFG1) & (~(0xF))) | 0x0, &pwm->PWM->TCFG1);

	writeb(readb(&pwm->PWM->TCON) | (1 << 3) , &pwm->PWM->TCON);

	writel(2000, &pwm->PWM->TCNTB0);

	writel(1000, &pwm->PWM->TCMPB0);

	
	writeb(readb(&pwm->PWM->TCON) | (1 << 1), &pwm->PWM->TCON);

	writeb(readb(&pwm->PWM->TCON) & (~(1 << 1)), &pwm->PWM->TCON);
	
	writeb(readb(&pwm->PWM->TCON) | 1, &pwm->PWM->TCON);

	return 0;
}

ssize_t pwm_read (struct file *f, char __user *p, size_t n, loff_t *off)
{
	
	printk("adfafdadfi %p \n", p); // 0xbebe 1c60  用户空间虚拟地址  
	return 0;
}


static ssize_t pwm_write (struct file *f, const char __user *p, size_t n, loff_t *off)
{
	return n;
}

static long pwm_ioctl(struct file *f, unsigned int cmd, unsigned long arg) 
{
	printk(" arg  %p \n", (void *)arg);


	return 0;
}

static int pwm_release(struct inode *i, struct file *f)
{
	
	writeb(readb(&pwm->PWM->TCON) & (~(0xF)), &pwm->PWM->TCON);

	return 0;
}
struct file_operations ops = {
	.open = pwm_open,
	.release = pwm_release,
	.unlocked_ioctl = pwm_ioctl,
	.read = pwm_read,
	.write = pwm_write,
}; 


static __init int pwm_init(void) 
{
	int ret = 0;
	pwm = kmalloc(sizeof(struct pwm_cdev), GFP_KERNEL);
	if(!pwm) {
		printk("kmalloc\n");
		return -ENOMEM;
	}

	ret = alloc_chrdev_region(&pwm->dev_no, 0, 1, "hello pwm");
	if(ret < 0) {
		goto err_alloc;
	}
	
	pwm->dev = cdev_alloc();
	if(pwm->dev < 0) {
		goto err_dev;
	}

	cdev_init(pwm->dev, &ops);
	ret = cdev_add(pwm->dev, pwm->dev_no, 1);
	if(ret < 0) {
		goto err_add;
	}
	pwm->GPD0_0 = ioremap(0x114000A0, sizeof(unsigned short)); 
	pwm->PWM = ioremap(0x139D0000, sizeof(struct pwm_s));
	
	return 0;
err_add:
	cdev_del(pwm->dev);
err_dev:
	unregister_chrdev_region(pwm->dev_no, 1);
err_alloc:
	kfree(pwm);

	return -1;
}

static __exit void pwm_exit(void)
{
	iounmap(pwm->GPD0_0);
	iounmap(pwm->PWM);
	cdev_del(pwm->dev);
	unregister_chrdev_region(pwm->dev_no, 1);
	kfree(pwm);
	return ;
}

module_init(pwm_init);
module_exit(pwm_exit);
