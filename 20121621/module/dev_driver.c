/* FPGA FND Ioremap Control
FILE : fpga_fpga_driver.c
AUTH : largest@huins.com */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>


#define IOM_FPGA_MAJOR 242		// ioboard fpga device major number
#define IOM_FPGA_NAME "dev_driver"		// ioboard fpga device name

#define IOM_FND_ADDRESS 0x08000004 // pysical address
#define IOM_FPGA_DOT_ADDRESS 0x08000210 // pysical address
#define IOM_LED_ADDRESS 0x08000016 // pysical address
#define IOM_TEXT_LCD_ADDRESS 0x08000090 // pysical address





//Global variable
static int dev_driver_usage = 0;
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;
unsigned char led_val[9] = { 0x00 ,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };
unsigned char fpga_number[11][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03}, // 9
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // blank
};


struct file *data_inode;
int fnd_val;
int fnd_ori_val;
int fnd_index;
char fnd_data[4];
int timer_interval;
int timer_cnt;

char lcd_val[33];
char lcd_ori_val[33];
char stu_num[8] = "20121621";
char stu_name[11] = "LeeSeokJune";
int num_flag = 0;
int name_flag = 0;
int num_cnt = 0;
int name_cnt = 0;

static struct struct_mydata {
	struct timer_list timer;
	int count;
};
struct struct_mydata mydata;

// define functions...
ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);

ssize_t fpga_dot_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);

ssize_t fpga_fnd_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
ssize_t fpga_fnd_read(struct file *inode, char *gdata, size_t length, loff_t *off_what);

ssize_t fpga_led_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
ssize_t fpga_text_lcd_write(struct file *inode, const char *gdata, size_t length, loff_t* off_what);
ssize_t timer_write(struct file *inode, const char *gdata, size_t length, loff_t* off_what);
void kernel_timer(unsigned long timerout);

void fpga_update();

int iom_fpga_open(struct inode *minode, struct file *mfile);
int iom_fpga_release(struct inode *minode, struct file *mfile);
long iom_fpga_ioctl(struct file *inode, unsigned ioctl_num, unsigned long ioctl_param);


// define file_operations structure 
struct file_operations iom_fpga_fops =
{
	.owner = THIS_MODULE,
	.open = iom_fpga_open,
	.write = iom_fpga_write,
	.unlocked_ioctl = iom_fpga_ioctl,
	.release = iom_fpga_release,
};

// when fnd device open ,call this function
int iom_fpga_open(struct inode *minode, struct file *mfile)
{
	if (dev_driver_usage != 0) return -EBUSY;

	dev_driver_usage = 1;

	return 0;
}

// when fnd device close ,call this function
int iom_fpga_release(struct inode *minode, struct file *mfile)
{
	dev_driver_usage = 0;

	return 0;
}

// when write to fnd device  ,call this function
ssize_t iom_fpga_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
	data_inode = inode;
	timer_interval = (int)gdata[0];
	timer_cnt = gdata[1];
	fnd_index = gdata[2];
	fnd_val = gdata[3];
	fnd_ori_val = gdata[3];
	
	
	
	fpga_fnd_write(data_inode, gdata, length, off_what);
	

	timer_write(data_inode,0,1,off_what);
	return 0;
}

ssize_t timer_write(struct file *inode, const char *gdata, size_t length, loff_t* off_what) {
	mydata.count = timer_cnt;
	printk("timer write\n");

	del_timer_sync(&mydata.timer);

	mydata.timer.expires = get_jiffies_64() + (timer_interval * HZ) / 10;
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = kernel_timer;

	add_timer(&mydata.timer);
	return 1;

}

void kernel_timer(unsigned long timeout) {
	struct struct_mydata *p_data = (struct struct_mydata*)timeout;

	printk("kernel_timer %d\n", p_data->count);
	
	p_data->count--;
	if (p_data->count < 0) { // cnt=0
		memset(fnd_data, 0, sizeof(fnd_data));
		fpga_fnd_write(data_inode, fnd_data, 4, 0);
		fnd_val = 0;
		fpga_led_write(data_inode, &led_val, 1, 0);
		fnd_val = 10;
		fpga_dot_write(data_inode, &fnd_val, 10, 0);
		memset(lcd_val, ' ', sizeof(lcd_val));
		fpga_text_lcd_write(data_inode, lcd_val, 33, 0);
		return;
	}

	//update
	fpga_update();
	fpga_led_write(data_inode, &led_val, 0, 1);
	fpga_dot_write(data_inode, &fnd_val, 10, 0);

	mydata.timer.expires = get_jiffies_64() + (timer_interval * HZ) / 10;
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = kernel_timer;

	add_timer(&mydata.timer);
}

void fpga_update() {
	
	int i;
	printk("update\n");
	fnd_val++;
	if (fnd_val > 8) {
		fnd_val = 1;
	}
	if (fnd_val == fnd_ori_val) {
		fnd_index++;
		if (fnd_index > 3)
			fnd_index = 0;
	}
	
	if (num_flag == 0 && num_cnt < 8) {
		num_cnt++;
		for (i = 0; i < 8; i++) {
			lcd_val[i + num_cnt] = lcd_ori_val[i];
		}
		lcd_val[num_cnt - 1] = ' ';
		if (num_cnt == 8) {
			num_flag = 1;
		}
	}
	else if (num_flag == 1 && num_cnt > 0) {
		num_cnt--;
		for (i = 0; i < 8; i++) {
			lcd_val[i + num_cnt] = lcd_ori_val[i];
		}
		lcd_val[num_cnt + 8] = ' ';
		if (num_cnt == 0) {
			num_flag = 0;
		}
	}
	
	if (name_flag == 0 && name_cnt < 5) {
		name_cnt++;
		for (i = 16; i < 27; i++) {
			lcd_val[i + name_cnt] = lcd_ori_val[i];
		}
		lcd_val[name_cnt + 15] = ' ';
		if (name_cnt == 5) {
			name_flag = 1;
		}
	}
	else if (name_flag == 1 && name_cnt > 0) {
		name_cnt--;
		for (i = 16; i < 27; i++) {
			lcd_val[i + name_cnt] = lcd_ori_val[i];
		}
		lcd_val[name_cnt + 11+16] = ' ';
		if (name_cnt == 0) {
			name_flag = 0;
		}
	}


	memset(fnd_data, 0, sizeof(fnd_data));
	fnd_data[fnd_index] = fnd_val;
	fpga_fnd_write(data_inode, fnd_data, 4, 0);
	fpga_led_write(data_inode, &led_val, 1, 0);
	fpga_dot_write(data_inode, &fnd_val, 10, 0);
	fpga_text_lcd_write(data_inode, &lcd_val, 32, 0);
	

}

ssize_t fpga_fnd_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
	int i;
	unsigned char value[4];
	unsigned short int value_short = 0;
	const char *tmp = gdata;
	
	for (i = 0; i < 4; i++) {
		value[i] = fnd_data[i];
	}
	value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
	outw(value_short, (unsigned int)iom_fpga_fnd_addr);

	return 0;

}

ssize_t fpga_led_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
	unsigned short _s_value;
	
	
	_s_value = (unsigned short)led_val[fnd_val];
	outw(_s_value, (unsigned int)iom_fpga_led_addr);



	return length;
}

ssize_t fpga_dot_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
	int i;


	for (i = 0; i < length; i++)
	{
		outw(fpga_number[fnd_val][i], (unsigned int)iom_fpga_dot_addr + i * 2);
	}

	return length;
}

ssize_t fpga_text_lcd_write(struct file *inode, const char *gdata, size_t length, loff_t* off_what) {
	int i;

	unsigned short int _s_value = 0;
	
	lcd_val[32] = ' ';

	
	
	for (i = 0; i < 32; i++)
	{
		_s_value = (lcd_val[i] & 0xFF) << 8 | lcd_val[i + 1] & 0xFF;
		outw(_s_value, (unsigned int)iom_fpga_text_lcd_addr + i);
		i++;
		
	}
	printk("-------------%s", lcd_val);

	return length;
	
}

long iom_fpga_ioctl(struct file *inode, unsigned int ioctl_num, unsigned long ioctl_param) {
	char* gdata = (char*)ioctl_param;
	data_inode = inode;
	timer_interval = (int)gdata[0];
	timer_cnt = gdata[1];
	fnd_index = gdata[2];
	fnd_val = gdata[3];
	fnd_ori_val = gdata[3];
	memset(fnd_data, 0, sizeof(fnd_data));
	fnd_data[fnd_index] = fnd_val;

	printk("----timer interval : %d, timer counter : %d, init value : %d, init index : %d\n", timer_interval, timer_cnt,fnd_val,fnd_index);
	
	
	fpga_fnd_write(data_inode, fnd_data, 4, 0);
	fpga_led_write(data_inode, &led_val, 1, 0);
	fpga_dot_write(data_inode, &fnd_val, 10, 0);
	timer_write(data_inode, 0, 1, 0);
	
	return 0;
}

int __init iom_fpga_fnd_init(void) //addressing
{
	int result,i;
	printk("init\n");
	result = register_chrdev(IOM_FPGA_MAJOR, IOM_FPGA_NAME, &iom_fpga_fops);
	if (result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	memset(lcd_val, ' ', sizeof(lcd_val));
	for (i = 0; i< 8; i++) {
		lcd_val[i] = stu_num[i];
		lcd_ori_val[i] = stu_num[i];
	}
	for (i = 16; i < 27; i++) {
		lcd_val[i] = stu_name[i - 16];
		lcd_ori_val[i] = stu_name[i - 16];
	}
	
	iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
	iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
	iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
	iom_fpga_text_lcd_addr = ioremap(IOM_TEXT_LCD_ADDRESS, 0x32);

	init_timer(&(mydata.timer));

	printk("init module, %s major number : %d\n", IOM_FPGA_NAME, IOM_FPGA_MAJOR);


	
	return 0;
}

void __exit iom_fpga_fnd_exit(void)
{
	printk("exit module\n");
	del_timer_sync(&mydata.timer);
	iounmap(iom_fpga_dot_addr);
	iounmap(iom_fpga_fnd_addr);
	iounmap(iom_fpga_led_addr);
	iounmap(iom_fpga_text_lcd_addr);

	unregister_chrdev(IOM_FPGA_MAJOR, IOM_FPGA_NAME);
}

module_init(iom_fpga_fnd_init);
module_exit(iom_fpga_fnd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
