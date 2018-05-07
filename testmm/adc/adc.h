
#ifndef __ADC_H__
#define __ADC_H__

#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/io.h>

struct my_adc {
	dev_t dev_no;
	struct cdev cdev;
	struct class *cls;
	struct device *test_device;


	wait_queue_head_t convert_queue;
	struct mutex lock;


	struct resource *irq_res;
	struct resource *adc_reg[5];

	unsigned long *ADCCON ;
	unsigned long *ADCDLY ;
	unsigned long *ADCDAT ;
	unsigned long *CLRINTADC;
	unsigned long *ADCMUX ;
};

irqreturn_t adc_complete(int n, void *arg);
static int adc_open (struct inode *inode, struct file *filp);
ssize_t adc_read (struct file *filp, char __user *p, size_t n, loff_t *off);
static ssize_t adc_write (struct file *filp, const char __user *p, size_t n, loff_t *off);
static long adc_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
static int adc_release (struct inode *inode, struct file *filp);
static int adc_mmap(struct file *filp, struct vm_area_struct *vma);
static struct my_adc *alloc_adc(void);
int init_adc(struct platform_device *pdev, struct my_adc *padc);
int adc_probe(struct platform_device *pdev);
int adc_remove(struct platform_device *pdev);


#endif /*__ADC_H__*/
