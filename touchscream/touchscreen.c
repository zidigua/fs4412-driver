

触摸屏的驱动源码：
	ts-gt818.c 
	
修改rootfs/etc/inittab目录下的文件
将tty0去掉，否则打印信息不正确。
	
	添加设备树信息： 
		
	i2c@13890000 {
		#address-cells = <1>;
		#size-cells = <0>;
		samsung,i2c-sda-delay = <100>;
		samsung,i2c-max-bus-freq = <20000>;
		pinctrl-0 = <&i2c3_bus>;
		pinctrl-names = "default";
		status = "okay";
		
		gt8818@5d{
			
			compatible = "gt8818";
			interrupt-parent = <&gpx0>;
        		interrupts = <4 1>;
			reg = <0x5d>;
		};

	};
 
	
[root@makeru ]# insmod ts-gt818.ko 
[   13.450000] /home/fengjunhui/kernel/ts-driver/ts-gt818.c:gt818_probe:200
[   13.485000] i2c_transfer is fail.
[   13.490000] retrying...........
[   13.505000] ts_write_cmd_end is success.
[   13.870000] input: Unspecified device as /devices/virtual/input/input1

	
	[root@makeru ]# [   45.410000] touch:(397,317)
[   45.445000] touch:(397,317)
[   45.500000] touch:(373,335)
[   45.535000] touch:(373,335)
[   45.590000] touch:(363,324)
[   45.625000] touch:(363,324)
	
	
	
[root@makeru ]# hexdump /dev/input/event1 
[  101.885000] touch:(188,370)
[  101.885000] touch:(20,279)
0000000 0065 0000 9d29 000d 0003 002f 0001 0000
0000010 0065 0000 9d29 000d 0003 0035 00bc 0000  ------- touch:(188,370)
0000020 0065 0000 9d29 000d 0003 0036 0172 0000
0000030 0065 0000 9d29 000d 0003 002f 0002 0000
0000040 0065 0000 9d29 000d 0003 0035 0014 0000
0000050 0065 0000 9d29 000d 0003 0036 0117 0000
0000060 0065 0000 9d29 000d 0000 0000 0000 0000
[  101.960000] touch:(188,370)
[  101.960000] touch:(8,274)	
	
	188 --->转化为16进制为 bc    0000010 0065 0000 9d29 000d 0003 0035 00bc 0000
	370 --->转化为16进制为 172   0000020 0065 0000 9d29 000d 0003 0036 0172 0000
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	




