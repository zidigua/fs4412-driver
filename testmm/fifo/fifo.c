#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/highmem.h> 
#include <asm/kmap_types.h>
#include <asm/page.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("xxxx");
MODULE_DESCRIPTION("xxxx");


struct my_fifo{
	dev_t dev_no;
	struct cdev cdev;
	struct file_operations *ops;
	char *pbuf;
	int front;
	int rear;
	int max_size;
};

static struct my_fifo fifo;

static int fifo_open (struct inode *inode, struct file *filp)
{
	filp->private_data = (void *)&fifo;


	return 0;
}

ssize_t fifo_read (struct file *filp, char __user *p, size_t n, loff_t *off)
{
	int ret = 0;
	struct my_fifo *f = (struct my_fifo *)(filp->private_data);
	int size = f->front - f->rear;


	if(size == 0) {
		//printk(" no data %d  \n ", __LINE__);
		return 0;
	} else if(n >= size) {		
		ret = copy_to_user(p, (f->pbuf)+f->rear, size);	
		fifo.rear = fifo.front;	
	//	printk(" copy to user counts %s \n", fifo.pbuf);
		return size;	
	} else  {
		ret = copy_to_user(p, &f->pbuf[f->rear], n);
		fifo.rear += n;
	//	printk(" copy to user counts -- %s \n ", fifo.pbuf);
		return n;
	}

}

static ssize_t fifo_write (struct file *filp, const char __user *p, size_t n, loff_t *off)
{
	int ret = 0;
//	struct my_fifo *fifo = (struct my_fifo *)(filp->private_data);
	printk("fifo _write \n");
	ret = copy_from_user(&fifo.pbuf[fifo.front], p, n);
	if(ret == 0) {	
		fifo.front += n;
		printk("fifo _write %d  \n", ret);
		return n;
	}
	return ret;
}

#define ON 0
#define OFF 1

static long fifo_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) 
{
#if 0
	struct my_fifo *fifo = (struct my_fifo *)(filp->private_data);
	switch(cmd) {
	case ON:
		 writeb(readb(fifo.reg_dat) | (1<<7), fifo.reg_dat);
		 break;
	case OFF:
		 writeb(readb(fifo.reg_dat) & (~(1<<7)), fifo.reg_dat);
		 break;
	}
#endif
	return 0;
}

static int fifo_release (struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

int fifo_mmap(struct file *filp, struct vm_area_struct *vma) 
{
	struct my_fifo *fifo = filp->private_data;
	return remap_pfn_range(vma, vma->vm_start, virt_to_phys(fifo->pbuf) >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);
}
struct file_operations fifo_ops = {
	.owner = THIS_MODULE,
	.open = fifo_open,
	.release = fifo_release,
	.read = fifo_read,
	.write = fifo_write,
	.unlocked_ioctl = fifo_ioctl,
	.mmap = fifo_mmap,
};

static __init int init_fifo(void) 
{
	int ret = 0;
	fifo.pbuf = (char *)__get_free_page(GFP_KERNEL);
	if(!fifo.pbuf) {
		printk("get page fail");
		return -ENOMEM;
	}
	ret =  alloc_chrdev_region(&fifo.dev_no, 0, 1, "hello fifo");
	if(ret < 0) {
		printk(" alloc_chrdev_region fail\n");
		goto mallo;
	}
	cdev_init(&fifo.cdev, &fifo_ops);
	fifo.cdev.owner = THIS_MODULE;
	ret = cdev_add(&fifo.cdev, fifo.dev_no, 1);
	if(ret < 0) {
		printk("cdev_add fail\n");
		goto add;
	}

	printk(" init fifo \n");

	fifo.max_size = 1 << PAGE_SHIFT;
	fifo.front = fifo.rear = 0;


	return ret;
add:
	unregister_chrdev_region(fifo.dev_no, 1);
mallo:
	free_page((unsigned long)fifo.pbuf);

	return -1;
}

static __exit void exit_fifo(void)
{

	cdev_del(&fifo.cdev);
	unregister_chrdev_region(fifo.dev_no, 1);
	free_page((unsigned long)fifo.pbuf);

	return;
}


module_init(init_fifo);
module_exit(exit_fifo);
