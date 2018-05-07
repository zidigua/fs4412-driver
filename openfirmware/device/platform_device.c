#include <linux/module.h>
#include <linux/platform_device.h>

struct platform_device *plat_dev;

struct resource led_res[] = {
	[0] = {
		.start = 0x11000c40,
		.end = 0x11000c47,
		.flags = IORESOURCE_MEM,
	},
	{}
};


static __init int init_led(void) 
{
	plat_dev = platform_device_alloc("plat_led2", -1);
	platform_device_add_resources(plat_dev, led_res, 1);
	platform_device_add(plat_dev);

	return 0;
}

static __exit void exit_led(void)
{
	platform_device_del(plat_dev);

}

module_init(init_led);
module_exit(exit_led);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("xxxx");
MODULE_DESCRIPTION("xxxx");
