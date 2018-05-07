#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/workqueue.h>


#include "clk.h"
#include "ov3640.h"

static unsigned int *GPJ0CON;
static unsigned int *GPJ1CON;
static unsigned int *GPL0CON;
static unsigned int *GPL0DAT;
static unsigned int *CISRCFMTn;
static unsigned int *CIGCTRLn;
static unsigned int *CIWDOFSTn;
static unsigned int *CIWDOFST2n; 
static unsigned int *CIOYSA1n;
static unsigned int *CIOYSA2n;
static unsigned int *CIOYSA3n;
static unsigned int *CIOYSA4n;
static unsigned int *CITRGFMTn; 
static unsigned int *CIOCTRLn;
static unsigned int *CISCPRERATIOn;
static unsigned int *CISCPREDSTn;
static unsigned int *CISCCTRLn;
static unsigned int *CITAREAn;
static unsigned int *CIIMGCPTn;
static unsigned int *ORGOSIZEn;
static unsigned int *CIFCNTSEQn;

wait_queue_head_t ov3640_wq;
int condition = 0;

struct video_device *vd;
int buf_size;
#define HSIZE 600
#define VSIZE 480

struct ov3640_fmt {
	const char *name;
	u32   fourcc;          /* v4l2 format id */
	u8    depth;
	bool  is_yuv;
};

static const struct ov3640_fmt formats[] = {
	{
		.name     = "4:2:2, packed, YUYV",
		.fourcc   = V4L2_PIX_FMT_YUYV,
		.depth    = 16,
		.is_yuv   = true,
	},
		{
		.name     = "RGB565",
		.fourcc   = V4L2_PIX_FMT_RGB565,
		.depth    = 16,
	},
	{
		.name	  = "RGB888",
		.fourcc   = V4L2_PIX_FMT_RGB24,
		.depth	  = 24,

	},

};

struct video_buffer{
	unsigned int virt_base;
	unsigned int phy_base;
};
struct video_buffer *buffer;

ssize_t ov3640_fops_read(struct file *file, char __user *buf, size_t count, loff_t *ops)
{
	int ret;
	//printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	wait_event_interruptible(ov3640_wq,condition);

	ret = copy_to_user(buf,(void *)buffer[0].virt_base,buf_size);
	if(ret){
		printk("copy_to_user is fail.\n");
		return 0;
	}
	
	condition = 0;
	
	return buf_size;
}

int ov3640_fops_open(struct file *file)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}
int ov3640_fops_release(struct file *file)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

static int ov3640_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	strcpy(cap->driver, "ov3640");
	strcpy(cap->card, "ov3640");
	cap->capabilities =  V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE;
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

static int ov3640_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	const struct ov3640_fmt *fmt;

	if (f->index >= ARRAY_SIZE(formats))
		return -EINVAL;

	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}
static int ov3640_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}
static int ov3640_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	if(f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE){
		return -EINVAL;
	}
	if((f->fmt.pix.pixelformat !=V4L2_PIX_FMT_YUYV)&& \
		(f->fmt.pix.pixelformat !=V4L2_PIX_FMT_RGB565)&& \
		(f->fmt.pix.pixelformat !=V4L2_PIX_FMT_RGB24)){
		return -EINVAL;
	}
	
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

static int ov3640_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	int ret = ov3640_try_fmt_vid_cap(file, priv, f);
	if (ret < 0)
		return ret;
	
	if(f->fmt.pix.height>0 &&f->fmt.pix.width>0){
		f->fmt.pix.height = VSIZE;
		f->fmt.pix.width = HSIZE;
	}
	
	if(f->fmt.pix.pixelformat ==V4L2_PIX_FMT_YUYV){
		f->fmt.pix.bytesperline = (f->fmt.pix.width * 16)>>3;
		f->fmt.pix.sizeimage = (f->fmt.pix.bytesperline * f->fmt.pix.height);
		buf_size = f->fmt.pix.sizeimage ;
		*CITRGFMTn &= ~(3<<29);
		*CITRGFMTn |= (2<<29);
	}else if(f->fmt.pix.pixelformat ==V4L2_PIX_FMT_RGB565){
		f->fmt.pix.bytesperline = (f->fmt.pix.width * 16)>>3;
		f->fmt.pix.sizeimage = (f->fmt.pix.bytesperline * f->fmt.pix.height);
		buf_size = f->fmt.pix.sizeimage ;
		*CITRGFMTn &= ~(3<<29);
		*CITRGFMTn |= (3<<29);

	}else if(f->fmt.pix.pixelformat ==V4L2_PIX_FMT_RGB24){
		f->fmt.pix.bytesperline = (f->fmt.pix.width * 24)>>3;
		f->fmt.pix.sizeimage = (f->fmt.pix.bytesperline * f->fmt.pix.height);
		buf_size = f->fmt.pix.sizeimage ;
		*CITRGFMTn &= ~(3<<29);
		*CITRGFMTn |= (3<<29);
		*CISCCTRLn |= (2<<11);
	}

	*CITRGFMTn |= (HSIZE<<16)|(VSIZE <<0);
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

struct v4l2_file_operations ov3640_fops = {
	.owner		= THIS_MODULE,
	.open           = ov3640_fops_open,
	.release        = ov3640_fops_release,
	.read           = ov3640_fops_read,
	.unlocked_ioctl = video_ioctl2, /* V4L2 ioctl handler */

};
int ov3640_reqbufs(struct file *file, void *priv,struct v4l2_requestbuffers *p)
{
	int i;
	printk("buf_size = %d\n",buf_size);
	buffer = kzalloc(4*sizeof(*buffer),GFP_KERNEL);
	for(i=0; i<4; i++){
		buffer[i].virt_base = __get_free_pages(GFP_KERNEL|GFP_DMA, get_order(buf_size));
		if(buffer[i].virt_base == (unsigned int)NULL){
			printk("alloc pages is fail\n");
			return -1;
		}
		buffer[i].phy_base = __virt_to_phys(buffer[i].virt_base);
	}

	//将物理地址写给控制器
	*CIOYSA1n = buffer[0].phy_base;
	*CIOYSA2n = buffer[1].phy_base;
	*CIOYSA3n = buffer[2].phy_base;
	*CIOYSA4n = buffer[3].phy_base;
	
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

int ov3640_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	*CISRCFMTn |= (HSIZE <<16)|(VSIZE <<0);
	*CIWDOFSTn |=(1<<31);
	*CIGCTRLn  |= (1<<29)|(1<<25)|(1<<20)|(1<<19)|(1<<16);
	*CIWDOFST2n = 0;
	*CIOCTRLn |= (1<<30);
	*CISCPRERATIOn = (10<<28)|(1<<16)|(1<<0);
	*CISCPREDSTn = (HSIZE<<16)|(VSIZE << 0);
	*CISCCTRLn |= (1<<30)|(1<<29)|(256<<16)|(256<<0);
	*CITAREAn = (HSIZE*VSIZE);

	*ORGOSIZEn = (VSIZE<<16)|(HSIZE<<0);
	*CIFCNTSEQn = (0x1);
	*CIIMGCPTn |= (1<<31)|(1<<30);
	*CISCCTRLn |= (1<<15);
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

int ov3640_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

const struct v4l2_ioctl_ops ov3640_ioctl_ops ={
	.vidioc_querycap      = ov3640_querycap,
		
	.vidioc_enum_fmt_vid_cap  = ov3640_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = ov3640_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = ov3640_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = ov3640_s_fmt_vid_cap,
	
	.vidioc_reqbufs       = ov3640_reqbufs,
#if 0
	.vidioc_querybuf      = vb2_ioctl_querybuf,
	.vidioc_qbuf          = vb2_ioctl_qbuf,
	.vidioc_dqbuf         = vb2_ioctl_dqbuf,
#endif

	.vidioc_streamon      = ov3640_streamon,
	.vidioc_streamoff     = ov3640_streamoff,
};

void ov3640_vd_release(struct video_device *vdev)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
}


int init_clk_camera(void)
{
	int ret;
	struct clk *fimc_clk,*cam_clk,*mpll_clk,*mount_clk;
	//1.摄像头的时钟24MHZ
	cam_clk = __clk_lookup("sclk_cam0");
	if(cam_clk == NULL){
		printk("get cam clk is fail\n");
		return -EAGAIN;
	}
	printk("src cam clk = %lu\n",_get_rate("sclk_cam0"));
	clk_prepare(cam_clk);
	clk_set_rate(cam_clk,24000000);
	clk_enable(cam_clk);
	printk("dest cam clk = %lu\n",_get_rate("sclk_cam0"));
	
	//2.控制器的时钟
	mpll_clk = __clk_lookup("mout_mpll_user_t");
	if(mpll_clk == NULL){
		printk("get mpll_clk clk is fail\n");
		return -EAGAIN;
	}
	mount_clk = __clk_lookup("mout_fimc0");
	if(mount_clk == NULL){
		printk("get mount_clk clk is fail\n");
		return -EAGAIN;
	}

	ret = clk_set_parent(mount_clk,mpll_clk);
	if(ret){
		printk("clk_set_parent is fail\n");
		return -EINVAL;

	}
	
	fimc_clk = __clk_lookup("sclk_fimc0");
	if(fimc_clk == NULL){
		printk("get fimc clk is fail\n");
		return -EAGAIN;
	}
	printk("src fimc clk = %lu\n",_get_rate("sclk_fimc0"));
	clk_prepare(fimc_clk);
	clk_set_rate(fimc_clk,160000000);
	clk_enable(fimc_clk);
	printk("dest fimc clk = %lu\n",_get_rate("sclk_fimc0"));

	mdelay(20);
	return 0;
}


void reset_camera(void)
{
	//1.摄像头的复位
	*GPL0DAT |= (1<<1);
	mdelay(10);
	*GPL0DAT &= ~(1<<1);
	mdelay(10);
	*GPL0DAT |= (1<<1);
	
	//2.控制器的复位(fimc)
	*CISRCFMTn |= (1<<31); //选择通讯协议
	*CIGCTRLn |= (1<<31);
	mdelay(10);
	*CIGCTRLn &= ~(1<<31);
	mdelay(10);
}

char ov3640_i2c_read(struct i2c_client *client,int reg)
{
	char ret,val;
	char reg_l = reg & 0xff;
	char reg_h = (reg >> 8) & 0xff;
	char w_buf[] = {reg_h,reg_l}; 
	struct i2c_msg read_reg[] = {
		[0] = {
			.addr = client->addr,
			.flags = 0, //先写
			.len = 2,
			.buf = w_buf,
		},
		[1] = {
			.addr = client->addr,
			.flags = I2C_M_RD, //后写
			.len = 1,
			.buf = &val,
		}
	};
	ret = i2c_transfer(client->adapter,read_reg,sizeof(read_reg)/sizeof(read_reg[0]));
	if(ret != 2){
		printk("transfer i2c msg is fail.\n");
		return -EAGAIN;
	}
	return val;
	
}

int ov3640_i2c_write(struct i2c_client *client,int reg,char val)
{
	int ret;
	char reg_l = reg & 0xff;
	char reg_h = (reg >> 8) & 0xff;
	char w_buf[] = {reg_h,reg_l,val}; 
	struct i2c_msg write_reg[] = {
		[0] = {
			.addr = client->addr,
			.flags = 0, 
			.len = 3,
			.buf = w_buf,
		},
	};
	ret = i2c_transfer(client->adapter,write_reg,sizeof(write_reg)/sizeof(write_reg[0]));
	if(ret != 1){
		printk("transfer i2c msg is fail ret = %d.\n",ret);
		return -EAGAIN;
	}
	return 0;

}

int init_camera(struct i2c_client *client)
{
	int i,ret;
	for(i=0; i<ARRAY_SIZE(ov3640_init_reg); i++)
	{
		ret = ov3640_i2c_write(client,ov3640_init_reg[i][0],ov3640_init_reg[i][1]);
		if(ret){
			printk("ov3640_i2c_write is fail\n");
			return -EAGAIN;
		}
		mdelay(1);
	}
	ret = ov3640_i2c_read(client,0x300a);
	printk("product id msb : %x\n",ret);
		ret = ov3640_i2c_read(client,0x300b);
	printk("product id lsb : %x\n",ret);
	return 0;
}
void clear_interrupt(void)
{
	*CIGCTRLn |= (1<<19); 
}
static irqreturn_t ov3640_interrupt(int irq, void *dev_id)
{
	clear_interrupt();

	//唤醒读
	wake_up(&ov3640_wq);
	condition = 1;
	return IRQ_HANDLED;
}

int ov3640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	int irq_num;
	struct device_node *node;
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);

	//1.分配结构体struct video_device
	vd = video_device_alloc();
	if(vd == NULL){
		printk("alloc video device is fail.\n");
		return -ENOMEM;
	}
	//2.结构体的填充
	strcpy(vd->name,"ov3640");
	vd->v4l2_dev = kzalloc(sizeof(struct v4l2_device),GFP_KERNEL);
	if(vd->v4l2_dev == NULL){
		printk("alloc v4l2_dev is fail.\n");
		return -ENOMEM;
	}
	strcpy(vd->v4l2_dev->name,"ov3640");
	ret = v4l2_device_register(NULL, vd->v4l2_dev);
	if(ret){
		printk("v4l2_device_register is fail.\n");
		return -EAGAIN;
	}
	vd->fops = &ov3640_fops;
	vd->ioctl_ops = &ov3640_ioctl_ops;
	vd->release = ov3640_vd_release;
	
	//3.硬件相关的操作
	//3.1gpio的初始化
	GPJ0CON = ioremap(0x11400240,4);
	GPJ1CON = ioremap(0x11400260,4);
	GPL0CON = ioremap(0x110000c0,4);
	GPL0DAT = ioremap(0x110000c4,4);
	CISRCFMTn = ioremap(0x11800000,4);
	CIGCTRLn = ioremap(0x11800008,4);
	CIWDOFSTn = ioremap(0x11800004,4);
	CIWDOFST2n = ioremap(0x11800014,4);
	CIOYSA1n = ioremap(0x11800018,4);
	CIOYSA2n = ioremap(0x1180001c,4);
	CIOYSA3n = ioremap(0x11800020,4);
	CIOYSA4n = ioremap(0x11800024,4);
	CITRGFMTn = ioremap(0x11800048,4);
	CIOCTRLn = ioremap(0x1180004c,4);
	CISCPRERATIOn = ioremap(0x11800050,4);
	CISCPREDSTn = ioremap(0x11800054,4);
	CISCCTRLn= ioremap(0x11800058,4);
	CITAREAn= ioremap(0x1180005c,4);
	CIIMGCPTn = ioremap(0x118000c0,4);
	ORGOSIZEn = ioremap(0x11800184,4);
	CIFCNTSEQn = ioremap(0x118001fc,4);
	
	*GPJ0CON = 0x22222222;
	*GPJ1CON = 0x2222;
	
	*GPL0CON &= ~((0xf<<4)|(0xf<<12));
	*GPL0CON |= (1<<4)|(1<<12);
	*GPL0DAT &= ~(1<<3);
	*GPL0DAT |= (1<<1);
	
	//3.2时钟初始化(摄像头硬件，fimc控制器)
	init_clk_camera();

	//3.3复位(摄像头硬件，fimc控制器)
	reset_camera();
	
	//3.4摄像头硬件初始化(数组)
	init_camera(client);
	
	//3.5申请中断
	init_waitqueue_head(&ov3640_wq);
	
	node = of_find_node_by_path("/camera/fimc@11800000");
	if(node == NULL){
		printk("of_find_node_by_path is fail.\n");
		return -EAGAIN;
	}

	irq_num = irq_of_parse_and_map(node,0);
	printk("irq_num = %d\n",irq_num);

	ret = request_irq(irq_num,ov3640_interrupt,IRQF_DISABLED,"ov3640-interrupt",NULL);
	if(ret){
		printk("request_irq is fail.\n");
		return -EAGAIN;
	}
	
	//4.注册结构体
	ret = video_register_device(vd, VFL_TYPE_GRABBER, 0);
	if(ret){
		printk("video_register_device is fail.\n");
		return -EAGAIN;
	}

	return 0;
	
}

int ov3640_remove(struct i2c_client *client)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	//注销结构体
	video_unregister_device(vd);
	return 0;
}

static const struct i2c_device_id ov3640_ids[] = {
	{ "0v3640", 0x3c},
	{},
};
MODULE_DEVICE_TABLE(i2c, ov3640_ids);

static struct of_device_id ov3640_of_match_table[] = {
	{ .compatible = "ov3640", },
	{ },
};
MODULE_DEVICE_TABLE(of, ov3640_of_match_table);

static struct i2c_driver ov3640_driver = {
	.driver = {
		.name = "ov3640",
		.owner = THIS_MODULE,
		.of_match_table = ov3640_of_match_table,
	},
	.probe = ov3640_probe,
	.remove	= ov3640_remove,
	.id_table = ov3640_ids,
};

module_i2c_driver(ov3640_driver);
MODULE_LICENSE("GPL");


