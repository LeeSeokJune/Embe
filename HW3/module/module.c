#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>


#define IOM_FND_ADDRESS 0x08000004 // pysical address


static int inter_major = 242, inter_minor = 0;
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;
static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static unsigned char *iom_fpga_fnd_addr;



irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);
static void kernel_timer(unsigned long timeout);
static void kernel_exit_timer(unsigned long timerout);
void timer_write(void);
void exit_timer_write(void);
ssize_t fpga_fnd_write();




static inter_usage = 0;
int interruptCount = 0;
char fnd_data[4];
int cnt=0,cnt_exit=0;
int exit_flag = 0;
int isPaused = 1;


wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);




static struct file_operations inter_fops =
{
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};

static struct struct_mydata {
	struct timer_list timer;
	int count;
};

struct struct_mydata mydata,exit_data;
int up_count;
ssize_t fpga_fnd_write(){
	int i;
	unsigned char value[4];
	unsigned short int value_short = 0;
	for (i = 0; i < 4; i++) {
		value[i] = fnd_data[i];
	}
	value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
	outw(value_short, (unsigned int)iom_fpga_fnd_addr);

	return 0;

}

irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) { // home button start
	printk(KERN_ALERT "home button!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

	if (isPaused == 1) {
		isPaused = 0;
		timer_write();
	}
																		 
/*
	
	*/
	

	return IRQ_HANDLED;
}

irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) { //back button pause
	isPaused = 1;
	printk(KERN_ALERT "back button!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	return IRQ_HANDLED;
}

irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg) { // vol+ button reset
	int i;
	printk(KERN_ALERT "vol+ button!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

	for (i = 0; i < 4; i++) {
		fnd_data[i] = 0;
	}
	cnt = 0;
	isPaused = 1;
	fpga_fnd_write();
	return IRQ_HANDLED;
}

irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) { // vol- button exit
	printk(KERN_ALERT "vol- button!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	
	if (exit_flag == 0) {
		if (gpio_get_value(IMX_GPIO_NR(5, 14)) == 0) {
			exit_flag = 1;
			exit_timer_write();
		}
	}
	else {
		if (gpio_get_value(IMX_GPIO_NR(5, 14))) {
			exit_flag = 0;
			cnt_exit = 0;
		}
	}

	return IRQ_HANDLED;
}


static int inter_open(struct inode *minode, struct file *mfile) {
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");

	// int1
	gpio_direction_input(IMX_GPIO_NR(1, 11));
	irq = gpio_to_irq(IMX_GPIO_NR(1, 11));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, inter_handler1, IRQF_TRIGGER_RISING, "home", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1, 12));
	irq = gpio_to_irq(IMX_GPIO_NR(1, 12));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, inter_handler2, IRQF_TRIGGER_RISING, "back", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2, 15));
	irq = gpio_to_irq(IMX_GPIO_NR(2, 15));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, inter_handler3, IRQF_TRIGGER_RISING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5, 14));
	irq = gpio_to_irq(IMX_GPIO_NR(5, 14));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, inter_handler4, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "voldown", 0);

	return 0;
}

static int inter_release(struct inode *minode, struct file *mfile) {
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {

	
	if (interruptCount == 0) {
		printk("sleep on\n");
		interruptible_sleep_on(&wq_write);
	}
	printk("write\n");
	return 0;
}
void timer_write(void) {
	del_timer_sync(&mydata.timer);

	mydata.timer.expires = get_jiffies_64() + (HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = kernel_timer;

	add_timer(&mydata.timer);

}
static void kernel_timer(unsigned long timeout) {
	if (isPaused) {
		return;
	}
	cnt++;
	int time_num[4], i;
	printk("------%d----\n", cnt);
	cnt = cnt % 10000;
	time_num[0] = cnt / 1000;
	time_num[1] = cnt / 100 - time_num[0] * 10;
	time_num[2] = cnt / 10 - time_num[0] * 100 - time_num[1] * 10;
	time_num[3] = cnt % 10;

	if (time_num[2] == 6) {
		time_num[2] = 0;
		time_num[1]++;
		cnt += 40;
	}

	if (time_num[0] == 6) {
		time_num[0] = 0;
		cnt -= 6000;
	}
	for (i = 0; i < 4; i++) {
		fnd_data[i] = time_num[i];
	}

	fpga_fnd_write();

	

	mydata.timer.expires = get_jiffies_64() + (HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = kernel_timer;

	add_timer(&mydata.timer);
}
void exit_timer_write(void) {
	del_timer_sync(&exit_data.timer);

	exit_data.timer.expires = get_jiffies_64() + (HZ);
	exit_data.timer.data = (unsigned long)&exit_data;
	exit_data.timer.function = kernel_exit_timer;

	add_timer(&exit_data.timer);
}
void kernel_exit_timer(unsigned long timerout) {
	if (exit_flag == 0)
		return;

	cnt_exit++;
	
	if (cnt_exit>= 3) {

		
		isPaused = 1;
		memset(fnd_data, 0, sizeof(fnd_data));
		fpga_fnd_write();

		del_timer_sync(&mydata.timer);
		__wake_up(&wq_write, 1, 1, NULL);
		return;
	}
	exit_data.timer.expires = get_jiffies_64() + (HZ);
	exit_data.timer.data = (unsigned long)&exit_data;
	exit_data.timer.function = kernel_exit_timer;

	add_timer(&exit_data.timer);
}

static int inter_register_cdev(void)
{
	int error;
	if (inter_major) {
		inter_dev = MKDEV(inter_major, inter_minor);
		error = register_chrdev_region(inter_dev, 1, "stopwatch");
	}
	else {
		error = alloc_chrdev_region(&inter_dev, inter_minor, 1, "stopwatch");
		inter_major = MAJOR(inter_dev);
	}
	if (error < 0) {
		printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", inter_major);
	cdev_init(&inter_cdev, &inter_fops);
	inter_cdev.owner = THIS_MODULE;
	inter_cdev.ops = &inter_fops;
	error = cdev_add(&inter_cdev, inter_dev, 1);
	if (error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}

static int __init inter_init(void) {
	int result;
	if ((result = inter_register_cdev()) < 0)
		return result;
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : 242 \n");
	iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);

	isPaused = 1;

	fnd_data[0] = 0;
	fnd_data[1] = 0;
	fnd_data[2] = 0;
	fnd_data[3] = 0;
	init_timer(&mydata.timer);
	init_timer(&exit_data.timer);
	fpga_fnd_write();
	return 0;
}

static void __exit inter_exit(void) {
	cdev_del(&inter_cdev);
	unregister_chrdev_region(inter_dev, 1);
	printk(KERN_ALERT "Remove Module Success \n");
	del_timer_sync(&mydata.timer);
	del_timer_sync(&exit_data.timer);
	iounmap(iom_fpga_fnd_addr);
}

module_init(inter_init);
module_exit(inter_exit);
MODULE_LICENSE("GPL");
