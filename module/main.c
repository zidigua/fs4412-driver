#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/io.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("zhur@farsight.com.cn");
MODULE_DESCRIPTION("a simple driver example!");

int n = 1;
module_param(n, int, 0644);
extern void test(void);

int *p;
static int mymodule_init(void)
{
	test();

	printk("module install\n");
	p = ioremap(0x11000c44, 4);
			*p = 1<<7;

			return 0;
}
		
static void mymodule_exit(void)
{
	printk("module release\n");
		*p = ~(1<<7);
		iounmap(p);
}

module_init(mymodule_init);
module_exit(mymodule_exit);


