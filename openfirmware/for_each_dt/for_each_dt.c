#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/platform_device.h>

int my_fn(struct device *dev, void *data)
{
	printk(" struct device %s  \n", dev->kobj.name);
	return 0;
}


static __init int dt_init(void) 
{
	
	struct device_node *child;
	struct property *pp;

	for_each_child_of_node(of_allnodes, child) {
		printk("%s \t\t\t %s  \n", child->name, child->full_name);

		for_each_property_of_node(child, pp) {
			const char *start = pp->name;
			if(!strcmp(start, "name")) {
				printk("\t\t\t %s   %s \n", start, (char *)pp->value);
			} else if(!strcmp(start, "reg")) {
				printk("\t\t\t %s  %#x \n", start, (unsigned long)pp->value);
			} else {
				printk("\t\t\t %s  \n", start);
			}
		}
	}

	bus_for_each_dev(&platform_bus_type, NULL, NULL, my_fn);

	printk("dt_init\n");
	
	
	return 0;
}

static __exit void dt_exit(void) 
{
	printk("dt_exit\n");


	
}

module_init(dt_init);
module_exit(dt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxxx");
MODULE_DESCRIPTION("low-level driver for AMD and Nvidia PATA IDE");
