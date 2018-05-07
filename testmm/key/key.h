
#ifndef __KEY_H__
#define __KEY_H__

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/io.h>

struct my_key {
	dev_t dev_no;
	struct cdev cdev;
	struct class *cls;
	struct device *test_device;

	struct timer_list my_timer;

	struct fasync_struct *fasyncqueue;

	atomic_t available;

	wait_queue_head_t convert_queue;
	struct mutex lock;

	struct resource *key_irq;

};

irqreturn_t key_complete(int n, void *arg);
static int key_open (struct inode *inode, struct file *filp);
ssize_t key_read (struct file *filp, char __user *p, size_t n, loff_t *off);
static ssize_t key_write (struct file *filp, const char __user *p, size_t n, loff_t *off);
static long key_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
static int key_release (struct inode *inode, struct file *filp);
static int key_fasync (int fd, struct file *filp, int mode);
static unsigned int key_poll(struct file *filp, struct poll_table_struct *wait);
static int key_mmap(struct file *filp, struct vm_area_struct *vma);
static struct my_key *alloc_key (const char *name);
int init_key(struct platform_device *pdev, struct my_key *pkey);
int key_probe(struct platform_device *pdev);
int key_remove(struct platform_device *pdev);


#endif /*__KEY_H__*/
