#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fb.h>
#include <linux/io.h> 
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>

#define BPP 24

static struct fb_info *fs_info = NULL;

//伪调色板16色
static unsigned int pseudo_palette[16+4];

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int fs4412_fimd_setcolreg(unsigned int regno, unsigned int red,
		unsigned int green, unsigned int blue,
		unsigned int transp, struct fb_info *info)
{
	unsigned int val;

	if (regno < 16) {
		unsigned int *pal = info->pseudo_palette;

		val  = chan_to_field(red, &info->var.red);
		val |= chan_to_field(green, &info->var.green);
		val |= chan_to_field(blue, &info->var.blue);

		pal[regno] = val;
	}

	return  0;
}

static struct fb_ops fs4412_fimd_ops = {
	.owner		= THIS_MODULE,	
	.fb_setcolreg	= fs4412_fimd_setcolreg,	
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static void fs4412_fimd_clk_enable(void)
{
	int val;

	unsigned int * __iomem clk_div_lcd;
	unsigned int * __iomem clk_src_lcd0;
	unsigned int * __iomem lcdblk_cfg;
	unsigned int * __iomem lcdblk_cfg2;

	//设置时钟slk_mpll  800M
	clk_div_lcd  = ioremap(0x1003c534,0x4);
	clk_src_lcd0 = ioremap(0x1003c234,0x4);
	lcdblk_cfg = ioremap(0x10010210, 0x4);
	lcdblk_cfg2= ioremap(0x10010214, 0x4);

	val = readl(clk_div_lcd);
	val &= ~0xf;
	writel(val, clk_div_lcd);

	val = readl(clk_src_lcd0);
	val &= ~0xf;
	val |= 0x6;
	writel(val, clk_src_lcd0);

	val = readl(lcdblk_cfg);
	val |= (1 << 1);
	writel(val, lcdblk_cfg);

	val = readl(lcdblk_cfg2);
	val |= (1 << 0);
	writel(val, lcdblk_cfg2);

	iounmap(clk_div_lcd);
	iounmap(clk_src_lcd0);
	iounmap(lcdblk_cfg);
	iounmap(lcdblk_cfg2);

	printk(KERN_INFO "======fimd clk enable.\n");
}

static void fs4412_fimd_vdd_set(void)
{
	unsigned int * __iomem gpf0con;
	unsigned int * __iomem gpf1con;
	unsigned int * __iomem gpf2con;
	unsigned int * __iomem gpf3con;

	gpf0con = ioremap(0x11400180,0x4);
	gpf1con = ioremap(0x114001A0,0x4);
	gpf2con = ioremap(0x114001C0,0x4);
	gpf3con = ioremap(0x114001E0,0x4);

	writel(0x22222222, gpf0con);
	writel(0x22222222, gpf1con);
	writel(0x22222222, gpf2con);
	writel(0x00222222, gpf3con);

	iounmap(gpf0con);
	iounmap(gpf1con);
	iounmap(gpf2con);
	iounmap(gpf3con);

	printk(KERN_INFO "======vdd lcd set done.\n");
}

static void fs4412_fimd_pwm_set(int on)
{
	int val;

	unsigned int * __iomem gpd0con;
	unsigned int * __iomem gpd0dat;

	gpd0con = ioremap(0x114000A0,0x4);
	gpd0dat = ioremap(0x114000A4,0x4);

	val = readl(gpd0con);
	val &= ~(0xf << 4);
	val |= (0x1 << 4);
	writel(val, gpd0con);

	val = readl(gpd0dat);
	val |= (on << 1);
	writel(val, gpd0dat);

	iounmap(gpd0con);
	iounmap(gpd0dat);

	printk(KERN_INFO "======pwm lcd set done.\n");
}

static void fb_fill_fix(struct  fb_info *info, unsigned int w, unsigned int h, unsigned int bpp)
{
	strncpy(info->fix.id ,"fslcd", 16);

	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = (bpp == 8 ? FB_VISUAL_PSEUDOCOLOR :
		FB_VISUAL_TRUECOLOR);

	info->fix.smem_len = PAGE_ALIGN(w*h*bpp/8);
	info->fix.line_length = w*bpp/8;

	info->fix.mmio_start = 0;
	info->fix.mmio_len = 0;

	info->fix.type_aux = 0;
	info->fix.xpanstep = 1;
	info->fix.ypanstep = 1;
	info->fix.ywrapstep = 0;
	info->fix.accel = FB_ACCEL_NONE;
}

static void fb_fill_var(struct  fb_info *info, unsigned int w, unsigned int h, unsigned int bpp)
{
	info->pseudo_palette = pseudo_palette; //伪16位调色板
	info->var.xres_virtual = w;
	info->var.yres_virtual = h;
	info->var.bits_per_pixel = bpp;
	info->var.accel_flags = FB_ACCELF_TEXT;
	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.activate = FB_ACTIVATE_NOW;
	info->var.height = -1;
	info->var.width = -1;

	switch (bpp) {
	case 16:
		info->var.red.offset = 11;
		info->var.green.offset = 5;
		info->var.blue.offset = 0;
		info->var.transp.offset = 0;
		info->var.red.length = 5;
		info->var.green.length = 6;
		info->var.blue.length = 5;
		info->var.transp.length = 0;
		break;
	case 24:
		info->var.red.offset = 16;
		info->var.green.offset = 8;
		info->var.blue.offset = 0;
		info->var.transp.offset = 0;
		info->var.red.length = 8;
		info->var.green.length = 8;
		info->var.blue.length = 8;
		info->var.transp.length = 0;
		break;
	case 32:
		info->var.red.offset = 16;
		info->var.green.offset = 8;
		info->var.blue.offset = 0;
		info->var.transp.offset = 24;
		info->var.red.length = 8;
		info->var.green.length = 8;
		info->var.blue.length = 8;
		info->var.transp.length = 8;
		break;
	default:
		break;
	};

	info->var.xres = w;
	info->var.yres = h;
}

static int __init fs4412_fimd_init(void)
{
	int ret, val;
	unsigned int size = PAGE_ALIGN(1024*600*BPP/8);
	dma_addr_t map_dma;

	unsigned int * __iomem vidcon0;
	unsigned int * __iomem vidcon1;
	unsigned int * __iomem vidcon2;
	unsigned int * __iomem vidtcon0;
	unsigned int * __iomem vidtcon1;
	unsigned int * __iomem vidtcon2;
	unsigned int * __iomem wincon0;
	unsigned int * __iomem vidoso0a;
	unsigned int * __iomem vidoso0b;
	unsigned int * __iomem vidoso0c;
	unsigned int * __iomem shaadowcon;
	unsigned int * __iomem winchmap2;
	unsigned int * __iomem vidw00add0b0;
	unsigned int * __iomem vidw00add1b0;
	unsigned int * __iomem win0map;
	unsigned int * __iomem vidw00add2;

	fs_info = framebuffer_alloc(0, NULL);
	if(!fs_info){
		printk("failed to allocate framebuffer \n");
		return -ENOENT;
	}

	fb_fill_fix(fs_info, 1024, 600, BPP);
	fb_fill_var(fs_info, 1024, 600, BPP);

	fs_info->fbops = &fs4412_fimd_ops;

	/* Fake palette of 16 colors */ 
	fs_info->screen_size  = 1024*600*BPP/8;
	printk("screen_size[%ld] = w[%d] * h[%d] * bpp[%d] / 8\n",
		 fs_info->screen_size, 1024, 600, BPP);

	fs4412_fimd_vdd_set();
	fs4412_fimd_clk_enable();
	fs4412_fimd_pwm_set(1);

	vidcon0  = ioremap(0x11c00000,0x4);
	vidcon1  = ioremap(0x11c00004,0x4);
	vidcon2  = ioremap(0x11c00008,0x4);
	vidtcon0 = ioremap(0x11c00010,0x4);
	vidtcon1 = ioremap(0x11c00014,0x4);
	vidtcon2 = ioremap(0x11c00018,0x4);

	win0map = ioremap(0x11C00180, 0x4);
	wincon0 = ioremap(0x11c00020, 0x4);

	vidoso0a = ioremap(0x11c00040,0x4);
	vidoso0b = ioremap(0x11c00044,0x4);
	vidoso0c = ioremap(0x11c00048,0x4);
	shaadowcon = ioremap(0x11c00034,0x4);
	winchmap2  = ioremap(0x11c0003c,0x4);
	vidw00add0b0 = ioremap(0x11c000a0,0x4);
	vidw00add1b0 = ioremap(0x11c000d0,0x4);
	vidw00add2 = ioremap(0x11C00104, 0x4);

	//vidcon0  [13-6]  VCLK = FIMD * SCLK/(CLKVAL+1
	//50M  = 800M/CLK_VAL+1  = 15
	writel(15 << 6, vidcon0);

	val = readl(vidcon1);
	val &= ~(1 << 7);	//Fetches video data at VCLK falling edge
#if 0
5 bit: Specifies VSYNC pulse polarity.
0 = Normal
1 = Inverted
6 bit: Specifies HSYNC pulse polarity.
	0 = Normal
	1 = Inverted
9~10bits: Specifies VCLK hold scheme at data under-flow.
00 = VCLK hold
01 = VCLK running
#endif
	val &= ~(3 << 9);
	val |= ((1 << 5)|(1 << 6)|(1 << 9));
	writel(val, vidcon1);
	
	//Reserved: This bit should be set to 1.
	writel(1 << 14, vidcon2);

#define  HSPW	10	//行同步
#define  HBPD	160
#define  HFPD	160
#define  VSPW	1	//帧同步
#define  VBPD	23
#define  VFPD	12
	val = readl(vidtcon0);
	val |= ((VBPD << 16)|(VFPD << 8)|(VSPW << 0));
	writel(val, vidtcon0);

	val = readl(vidtcon1);
	val |= ((HBPD << 16)|(HFPD << 8)|(HSPW << 0));
	writel(val, vidtcon1);

#if 0
Determines horizontal & vertical size of display. 
NOTE: HOZVAL = (Horizontal display size) – 1 and LINEVAL = (Vertical display size) – 1.
#endif
	val = ((1023 << 0) | (599 << 11));
	writel(val, vidtcon2);

	val = readl(wincon0);
	val &= ~(0x3 << 9); //Selects DMA Burst Maximum Length. 00 = 16 word-burst
	writel(val, wincon0);

#if 0
6bit: Selects blending category 
1 = Per pixel blending
#endif
	val |= (0x1 << 6);
	writel(val, wincon0);
#if 0
1bit: When per pixel blending (BLD_PIX == 1):
0 = Selected by AEN (A value)
#endif
	val &= ~(0x1 << 1);
	writel(val, wincon0);

	val &= ~(0xf << 2);
	if (BPP == 16){
		// 0101 = 16 BPP (non-palletized, R:5-G:6-B:5)
		val |= (0x5 << 2);
	} else if(BPP == 24){
		//1011 = Unpacked 24 BPP (non-palletized R:8-G:8-B:8)
		val |= (0xE << 2);
	}else if(BPP == 32){
		val |= (0xD << 2);
	}else{
		return -EINVAL;
	}
	writel(val, wincon0);

	writel(0xF, win0map);

	fs_info->screen_base = dma_alloc_writecombine(fs_info->dev, size,
					&map_dma, GFP_KERNEL);

	if(!fs_info->screen_base) {
		printk(KERN_ERR "dma_alloc_writecombine fail \n");
		return -ENOMEM;
	}

	memset(fs_info->screen_base, 0x0, size);
	fs_info->fix.smem_start = map_dma;

	writel(fs_info->fix.smem_start, vidw00add0b0);
	writel(fs_info->fix.smem_start+fs_info->fix.smem_len, vidw00add1b0);
	writel((0 << 13) | ((1024 * BPP / 8)<< 0), vidw00add2);

	// Specifies horizontal & vertical screen coordinate for left top pixel of OSD image.
	writel(0, vidoso0a);

	// Specifies horizontal & vertical screen coordinate for right bottom pixel of OSD image.
	if(BPP == 16){
		val = (((1023*2) << 11) | ((599*2) << 0));
	}else if((BPP == 24) || (BPP == 32)){
		val = (((1023) << 11) | ((599) << 0));
	}
	writel(val, vidoso0b);

	//Specifies the Window Size For example, Height Width (number of word)
	val = (1024*600);
	writel(val, vidoso0c);

	//0: Enables Channel 0.
	val = (1 << 0);
	writel(val, shaadowcon);

	//Enables the video output and video control signal
	val = readl(vidcon0);
	val |= (3 << 0);
	writel(val, vidcon0);
	val = readl(wincon0);
	val |= (1 << 0);
	writel(val, wincon0);

	iounmap(vidcon0);
	iounmap(vidcon1);
	iounmap(vidcon2);
	iounmap(vidtcon0);
	iounmap(vidtcon1);
	iounmap(vidtcon2);

	iounmap(win0map);
	iounmap(wincon0);

	iounmap(vidoso0a);
	iounmap(vidoso0b);
	iounmap(vidoso0c);

	iounmap(shaadowcon);
	iounmap(winchmap2);
	iounmap(vidw00add0b0);
	iounmap(vidw00add1b0);
	iounmap(vidw00add2);

	ret = register_framebuffer(fs_info);
	if(0 > ret){
		printk(KERN_ERR "failed to register framebuffer \n");
		dma_free_coherent(fs_info->dev, fs_info->fix.smem_len,
				  fs_info->screen_base, fs_info->fix.smem_start);
		return ret;
	}

	printk(KERN_INFO "fs4412_fimd_init done. \n");

	return  0;
}

static void __exit fs4412_fimd_exit(void)
{
	printk(KERN_INFO "fs4412_fimd_exit ... \n");
	fs4412_fimd_pwm_set(0);
	if(fs_info){
		unregister_framebuffer(fs_info);
		if (fs_info->cmap.len)
			fb_dealloc_cmap(&fs_info->cmap);
		dma_free_coherent(fs_info->dev, fs_info->fix.smem_len,
				  fs_info->screen_base, fs_info->fix.smem_start);
		framebuffer_release(fs_info);
	}
	printk(KERN_INFO "fs4412_fimd_exit done. \n");
}

module_init(fs4412_fimd_init);
module_exit(fs4412_fimd_exit);

MODULE_LICENSE("GPL");

