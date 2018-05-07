#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/sched.h>

static inline void sleep(unsigned sec)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(sec * HZ);
}

static __init int init_module1(void)
{
	unsigned long atfer_time = jiffies + HZ;

	int num = 0;
	sleep(1);

	while (time_after(jiffies, atfer_time)) {
		printk("------------\n");
		sleep(1);
		num ++;
		if(num > 20) {
			break;
		}
	}


	return 0;
}

static __exit void exit_module1(void)
{
	printk("-------exit_module-----\n");
}

module_init(init_module1);
module_exit(exit_module1);

MODULE_LICENSE("GPL");