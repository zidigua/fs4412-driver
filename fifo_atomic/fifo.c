#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h> 
#include <linux/highmem.h> 
#include <asm/kmap_types.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("zhur@farsight.com.cn");
MODULE_DESCRIPTION("a simple driver example!");

struct fifo{
	dev_t num;
	struct cdev cdev;

//	int available;
	atomic_t available;
//	struct mutex lock;
//	spinlock_t lock;

	//queue property
	char *qbuf;
	int start; //pos
	int end;   //pos
	int cursize;
	int maxsize;
}FIFO;

static int fifo_open(struct inode *inodep, struct file *filep)
{	
//	printk("fifo open called\n");
//	local_irq_disable();
//	mutex_lock(&FIFO.lock);
//	spin_lock(&FIFO.lock);
//	if(FIFO.available)
//	{
//		FIFO.available = 0;
	
//		local_irq_enable();
//	mutex_unlock(&FIFO.lock);
//	spin_unlock(&FIFO.lock);

	if(0 == atomic_dec_and_test(&FIFO.available))
		return 0;
//	}

//	local_irq_enable();
//	mutex_unlock(&FIFO.lock);
//	spin_unlock(&FIFO.lock);
	atomic_inc(&FIFO.available, 1);
	return -EBUSY;
}

static ssize_t fifo_read (struct file *f, char __user *p, size_t n, loff_t *off)
{
	int size = n>FIFO.cursize?FIFO.cursize:n;
	if(size == 0)
		return 0;
	int len1, len2;
	if(FIFO.start <= FIFO.end)// copy once
	{
		if(copy_to_user(p, FIFO.qbuf, size))
		{
			printk("copy to user fail1\n");
			return -EINVAL;
		}
	}
	else
	{
		len1 = FIFO.maxsize-FIFO.start;
		if(copy_to_user(p, &FIFO.qbuf[FIFO.start], len1))
		{
			printk("copy to user fail2\n");
			return -EINVAL;
		}
		len2 = size-len1;
		if(copy_to_user(p+len1, FIFO.qbuf, len2))
		{
			printk("copy to user fail3\n");
			return -EINVAL;
		}
	}

	FIFO.start = (FIFO.start+size)%FIFO.maxsize;
	FIFO.cursize -= size;
	
	
	return size;
}

static ssize_t fifo_write (struct file *f, const char __user *p, size_t n, loff_t *off)
{
	int space = FIFO.maxsize - FIFO.cursize;
	int size = n>space?space:n;
	int len1 = (FIFO.maxsize-1 - FIFO.end);
	int len2;
	if(!space)
		return 0;
	
//	copy_from_user(&FIFO.qbuf[end+1], p, size);
	if(len1 >= size)//copy once
	{
		if(copy_from_user(&FIFO.qbuf[FIFO.end+1], p, size))
		{
			printk("copy from user fail1\n");
			return -EINVAL;
		}
	}
	else
	{
		if(copy_from_user(&FIFO.qbuf[FIFO.end+1], p, len1))
		{
			printk("copy from user fail2\n");
			return -EINVAL;
		}
		len2 = size-len1;
		if(copy_from_user(FIFO.qbuf, p+len1,  len2))
		{
			printk("copy from user fail3\n");
			return -EINVAL;
		}
	}
	FIFO.end = (FIFO.end+size)%FIFO.maxsize;
	FIFO.cursize += size;
	
	return size;
}

static long fifo_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int fifo_release(struct inode *inodep, struct file *filep)
{
//	printk("fifo closed\n");
//	FIFO.available = 1;
	atomic_inc(&FIFO.available, 1);
	return 0;
}

int fifo_mmap(struct file *f, struct vm_area_struct *vma)
{
	return remap_pfn_range(vma, vma->vm_start, virt_to_phys(FIFO.qbuf)>>PAGE_SHIFT, 
		vma->vm_end-vma->vm_start, 
		vma->vm_page_prot);
}

struct file_operations myops = {
	.owner = THIS_MODULE,
	.open = fifo_open,
	.unlocked_ioctl  = fifo_ioctl,
	.read = fifo_read,
	.write = fifo_write,
	.mmap = fifo_mmap,
	.release = fifo_release
};

static int __init mymodule_init(void)
{
	int ret = 0;
	printk("fifo module in\n");

//	FIFO.available = 1;
	atomic_init(&FIFO.available, 1);
//	mutex_init(&FIFO.lock);	
//	spin_lock_init(&FIFO.lock);	

	FIFO.qbuf = (char*)__get_free_page(GFP_KERNEL);
	FIFO.maxsize = 1<<PAGE_SHIFT;
	FIFO.cursize = FIFO.start = 0;
	FIFO.end = -1;

	//request dev num
	ret = alloc_chrdev_region(&FIFO.num, 0, 1, "fifodddddddddd");
	if(ret)
	{
		printk("devnum alloc fail!\n");
		return ret;
	}
	printk("num: %d\n", MAJOR(FIFO.num) );

	//create cdev object
#if 0
	FIFO.mycdev =  cdev_alloc();
	if(NULL==fifo.mycdev)
	{
		printk("alloc cdev fail!\n");
		goto cdev_alloc_out;
	}
#endif
	//init cdev ops
	cdev_init(&FIFO.cdev,  &myops);
	FIFO.cdev.owner = THIS_MODULE;

	//register cdev into kernel
	ret = cdev_add(&FIFO.cdev, FIFO.num, 1);
	if(ret)
	{
		printk("add cdev fail!\n");
		goto cdev_add_out;
	}

	return 0;

cdev_add_out:
#if 0
	kfree(FIFO.mycdev);
#endif
//cdev_alloc_out:
	unregister_chrdev_region(FIFO.num, 1);

	return ret;
}

static void __exit mymodule_exit(void)
{
	//unregister cdev from kernel
	cdev_del(&FIFO.cdev);
#if 0
	//release cdev object
	kfree(&FIFO.cdev);
#endif
	//release dev num
	unregister_chrdev_region(FIFO.num, 1); 
	
	free_page((unsigned long)FIFO.qbuf);
	printk("fifo module release\n");
}


module_init(mymodule_init);
module_exit(mymodule_exit);
