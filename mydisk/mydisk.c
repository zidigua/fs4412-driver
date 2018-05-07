//---mydisk.c   实现最简功能
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/vmalloc.h>

#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/platform_device.h>


#define MYDISK_MINORS		4   //硬盘的分区数
#define MYDISK_HEADS		4
#define	MYDISK_SECTORS		16      //磁盘的扇区数
#define MYDISK_CYLINDERS		256   //柱面数（多个磁盘的磁道叠加形成柱面）
#define MYDISK_SECTOR_SIZE	512   //磁盘扇区大小
#define MYDISK_SECTOR_TOTAL	(MYDISK_HEADS * MYDISK_SECTORS * MYDISK_CYLINDERS)
#define MYDISK_SIZE		(MYDISK_SECTOR_TOTAL * MYDISK_SECTOR_SIZE)

static int mydisk_major = 0;
static char mydisk_name[] = "mydisk";
static struct gendisk *mydisk = NULL;
spinlock_t mylock;

char data[MYDISK_SIZE] = {0};


int led_pin;
int need_schedule_help;


static int mydisk_open(struct block_device *bdev, fmode_t mode)
{
	return 0;
}

static void mydisk_release(struct gendisk* gd, fmode_t mode)
{
	return ;
}

static int mydisk_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg)
{
	return 0;
}

//获取磁盘几何信息
static int mydisk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->cylinders = MYDISK_CYLINDERS;
	geo->heads = MYDISK_HEADS;
	geo->sectors = MYDISK_SECTORS;
	geo->start = 0;
	return 0;
}

static void mydisk_request(struct request_queue *q)
{
	int status = 0;
	struct request *req;
	struct bio *bio;
	struct bio_vec bvec;
	struct bvec_iter iter;
	sector_t sector;
	unsigned char *buffer;
	unsigned long offset;
	unsigned long nbytes;
	struct mydisk_dev *bdev;

#if 1	//灯泡 闪烁
	static int busy_flash = 0;
	busy_flash ++;
	if(busy_flash%7 == 0) {
		gpio_set_value(led_pin, 1);
	} else {
		gpio_set_value(led_pin, 0);
	}
#endif
	printk("dealing !\n");
	bdev = q->queuedata;
	req = blk_fetch_request(q);
	/* 第一层遍历，遍历每一个request */
	while (req != NULL) {
		/* 第二层遍历，遍历每一个bio */
		__rq_for_each_bio(bio, req) {
			sector = bio->bi_iter.bi_sector;
			if (bio_end_sector(bio) > get_capacity(mydisk)) {
				status = -EIO;
				goto out;
			}

			bio_for_each_segment(bvec, bio, iter) {
				buffer = kmap_atomic(bvec.bv_page);
				offset = sector * MYDISK_SECTOR_SIZE;
				nbytes = bvec.bv_len;

				if (bio_data_dir(bio) == WRITE)
					memcpy(data + offset, buffer + bvec.bv_offset, nbytes);
				else
					memcpy(buffer + bvec.bv_offset, data + offset, nbytes);

				kunmap_atomic(buffer);
				sector += nbytes >> 9;
			}

			status = 0;
		}
out:
		if (!__blk_end_request_cur(req, status))
			req = blk_fetch_request(q);
	}
}

static struct block_device_operations mydisk_fops = {
	.owner = THIS_MODULE,
	.open = mydisk_open,
	.release = mydisk_release,
	.ioctl = mydisk_ioctl,
	.getgeo = mydisk_getgeo,
};

void my_make_request_func(struct request_queue *q, struct bio *bio)
{

	unsigned char *buffer;
	unsigned long offset;
	unsigned long nbytes;
	struct bio_vec bvec;
	struct bvec_iter iter;
//	printk("bio %s !\n", (bio_data_dir(bio) == WRITE)?"write":"read");
	sector_t sector = bio->bi_iter.bi_sector;
	bio_for_each_segment(bvec, bio, iter) {
		buffer = kmap_atomic(bvec.bv_page);
		offset = sector * MYDISK_SECTOR_SIZE;
		nbytes = bvec.bv_len;

		if (bio_data_dir(bio) == WRITE)
			memcpy(data + offset, buffer + bvec.bv_offset, nbytes);
		else
			memcpy(buffer + bvec.bv_offset, data + offset, nbytes);

		kunmap_atomic(buffer);
		sector += nbytes >> 9;
	}


	return  bio_endio(bio, 0);
}

static int  mydisk_init(void)
{

	// 申请主设备号，如果参数为0，表示自动分配一个主设备号
	mydisk_major = register_blkdev(mydisk_major, mydisk_name);
	if (mydisk_major <= 0) {
		printk(KERN_WARNING "mydisk: unable to get major number\n");
		return -EBUSY;
	}

	mydisk = alloc_disk(MYDISK_MINORS);
	if (!mydisk) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		return -1;
	}

	spin_lock_init(&mylock);


/*
#if 0 //need schedule help
	mydisk->queue = blk_init_queue(mydisk_request, &mylock);
#else
//	blk_queue_logical_block_size(bdev->queue, MYDISK_SECTOR_SIZE);
	mydisk->queue = blk_alloc_queue(GFP_KERNEL);
	blk_queue_make_request(mydisk->queue, my_make_request_func);
#endif
*/
	if(!need_schedule_help) {
		mydisk->queue = blk_init_queue(mydisk_request, &mylock);
	} else {
		//	blk_queue_logical_block_size(bdev->queue, MYDISK_SECTOR_SIZE);
		mydisk->queue = blk_alloc_queue(GFP_KERNEL);
		blk_queue_make_request(mydisk->queue, my_make_request_func);
	}


	mydisk->major = mydisk_major;
	mydisk->first_minor = 0;
	mydisk->fops = &mydisk_fops;
	snprintf(mydisk->disk_name, 32, "mydisk%c", 'a');
	set_capacity(mydisk, MYDISK_SECTOR_TOTAL);

	add_disk(mydisk);
	return 0;

}

static void mydisk_exit(void)
{
	del_gendisk(mydisk);
//	put_disk(mydisk);
	blk_cleanup_queue(mydisk->queue);
	unregister_blkdev(mydisk_major, "mydisk");
}


static int my_disk_probe(struct platform_device *pdev)
{
	led_pin = of_get_gpio(pdev->dev.of_node, 0);
	printk("pin: %d \n", led_pin);
	devm_gpio_request(&pdev->dev, led_pin, "led pin");
	gpio_direction_output(led_pin, 1);
	gpio_set_value(led_pin, 0);

	of_property_read_u32(pdev->dev.of_node, "need_schdule_help", &need_schedule_help);
	printk("need_schedule_help  %d \n", need_schedule_help);

	 mydisk_init();

	return 0;
}

static int my_disk_remove(struct platform_device *pdev)
{
	mydisk_exit();
	return 0;
}



static struct of_device_id my_disk_table[] = {
	{ .compatible = "disk" },
	{ }
};

static struct platform_driver my_disk = {
	.probe = my_disk_probe,
	.remove = my_disk_remove,
	.driver = {
		.name = "my_disk",
		.of_match_table = of_match_ptr(my_disk_table)
	}
};


static int __init my_module_init(void)
{
	return platform_driver_register(&my_disk);
}

static void __exit my_module_exit(void)
{
	platform_driver_unregister(&my_disk);
}

module_init(my_module_init);
module_exit(my_module_exit);


MODULE_LICENSE("Dual BSD/GPL");

