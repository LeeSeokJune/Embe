/* FPGA FND Test Application
File : fpga_test_fnd.c
Auth : largest@huins.com */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>


#define MAX_DIGIT 4
#define FPGA_DEVICE "/dev/dev_driver"

int main(int argc, char **argv)
{
	int dev;
	char data[4];
	unsigned char retval;
	int i;
	int init_value;
	char init_num;
	unsigned char timer_interval = 0, timer_counter = 0, timer_init = 0;
	

	memset(data, 0, sizeof(data));

	// param check
	if (argc != 4 || strlen(argv[3]) != 4) {
		printf("Error! Invalid Value!\n");
		printf("ex) ./app [1-100] [1-100] [0001-8000]\n");
		return -1;
	}

	timer_interval = atoi(argv[1]);
	timer_counter = atoi(argv[2]);

	if (timer_interval < 1 || timer_interval > 100) {
		printf("Error! Invalid Value!\n");
		printf("ex) ./app [1-100] [1-100] [0001-8000]\n");
		return -1;
	}
	if (timer_counter < 1 || timer_counter > 100) {
		printf("Error! Invalid Value!\n");
		printf("ex) ./app [1-100] [1-100] [0001-8000]\n");
		return -1;
	}

	for (i = 0; i < strlen(argv[3]); i++) {
		if (argv[3][i] != '0') {
			init_num = i;
			init_value = argv[3][i] - 0x30;
			break;
		}
	}

	if (init_value < 1 || init_value>8) {
		printf("Error! Invalid Value!\n");
		printf("ex) ./app [1-100] [1-100] [0001-8000]\n");
	}

	dev = open(FPGA_DEVICE, O_RDWR);
	if (dev < 0) {
		printf("Device open error : %s\n", FPGA_DEVICE);
		exit(1);
	}

	//printf("timer interval : %d, timer counter : %d, init value : %d, init index : %d\n", timer_interval, timer_counter, init_value, init_num);

	data[0] = (char)timer_interval;
	data[1] = (char)timer_counter;
	data[2] = (char)init_num;
	data[3] = (char)init_value;
	
	ioctl(dev, _IOW(242, 0, char*), data);

	close(dev);

	return(0);
}
