#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include "fpga_dot_font.h"


#define BUFF_SIZE 64
#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16

#define KEY_RELEASE 0
#define KEY_PRESS 1

#define KEY_BACK 158
#define KEY_VOLUP 115
#define KEY_VOLDOWN 114

#define QUIT 0 
#define DEFAULT_MODE 1
#define CLOCK_MODE 2
#define COUNTER_MODE 3
#define TEXTEDITOR_MODE 4
#define DRAWBOARD_MODE 5


unsigned char dot_mode[3][10] = {	// for text editor mode
	{0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63},	// A
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e},	// 1
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

unsigned char txt_list[9][3] = {	// for text editor mode
	".QZ","ABC","DEF","GHI","JKL","MNO","PRS","TUV","WXY"
};

int mode_num;
key_t mode_key, input_key, output_key;
int mode_memory_id, input_memory_id, output_memory_id;
char *input_memory, *output_memory;
int *mode_memory;

int shared_memory() {
	printf("shared_memory\n");
	// shared_memory 생성
	mode_key = 1;
	input_key = 2;
	output_key = 3;
	
	if ((mode_memory_id = shmget(mode_key, 1, IPC_CREAT | 0600)) == -1)
		printf("mode shmget\n");
	if ((input_memory_id = shmget(input_key, 64, IPC_CREAT | 0660)) == -1)
		printf("input shmget\n");
	if ((output_memory_id = shmget(output_key, 128, IPC_CREAT | 0666)) == -1)
		printf("output shmget\n");

	// attach the segments to memory space
	if ((mode_memory = shmat(mode_memory_id, NULL, 0)) == (int *)-1)
		printf("mode shmat\n");
	if ((input_memory = shmat(input_memory_id, NULL, 0)) == (char *)-1)
		printf("input shmat\n");
	if ((output_memory = shmat(output_memory_id, NULL, 0)) == (char *)-1)
		printf("output shmat\n");

	// initialize data to default
	*mode_memory = 1;
	init_memory();

	return 0;
	
}

int init_memory() {
	
	int i = 0;
	time_t clock = time(NULL);
	struct tm *ctime;
	for (i = 0; i < 9; i++) {
		input_memory[i] = 0;
	}
	for (i = 0; i < 20; i++) {
		output_memory[i] = 0;
	}

	if (*mode_memory == CLOCK_MODE) {
		ctime = localtime(&clock);
		output_memory[0] = ctime->tm_hour / 10;
		output_memory[1] = ctime->tm_hour % 10;
		output_memory[2] = ctime->tm_min / 10;
		output_memory[3] = ctime->tm_min % 10;
		output_memory[4] = 128;
		output_memory[5] = 0;
	}
	if (*mode_memory == COUNTER_MODE) {

		output_memory[4] = 10; // 10진법
		output_memory[5] = 64; // 2번째 led
	}
	if (*mode_memory == TEXTEDITOR_MODE) {
		for (i = 0; i < 9; i++) {
			output_memory[i + 4] = ' ';
		}
	}

	printf("init_memory\n");
	return 0;
}



int main_process(void) {
	printf("main\n");
	while (1) {

		switch (*mode_memory) {
			case QUIT:
				return 0;
			case CLOCK_MODE:
				Clock_mode();
				init_memory();
				break;
			case COUNTER_MODE:
				Counter_mode();
				init_memory();
				break;
			case TEXTEDITOR_MODE:
				Text_editor_mode();
				break;
			case DRAWBOARD_MODE:
				Draw_board_mode();
				break;
		}
	}
}

int event0_process() {
	struct input_event ev[BUFF_SIZE];
	int fd, rd, size = sizeof(struct input_event);
	
	int i;
	int cnt = 1;


	printf("event0 process\n");
	char* device = "/dev/input/event0";
	if ((fd = open(device, O_RDONLY)) == -1) { 
		printf("%s is not a vaild device\n", device);
	}
	
	
	
	while (*mode_memory != 0) {
		if ((rd = read(fd, ev, size * BUFF_SIZE)) < size)
		{
			printf("read()");
			return (0);
		}


		if (ev[0].value != ' ' && ev[1].value == 1 && ev[1].type == 1) { // Only read the key press event
			printf("code%d\n", (ev[1].code));
		}

		if (ev[0].value == KEY_RELEASE) { 
			if (ev[0].code == KEY_VOLUP) {// volup btn
				cnt++;
				if (cnt > 5)
					cnt = 2;
				*mode_memory = cnt;
				init_memory();
			}

			else if (ev[0].code == KEY_VOLDOWN) { //voldown btn
				cnt--;
				if (cnt < 2)
					cnt = 5;
				*mode_memory = cnt;
				init_memory();
			}
			else if (ev[0].code == KEY_BACK)
				*mode_memory = QUIT;
			
		}
	}
	close(fd);
	return 0;
}

int input_process() {

	int fd1, rd1,i;
	int flag = 0;
	char* device2 = "/dev/fpga_push_switch";
	unsigned char push_switch[9];
	printf("input_process\n");
	if ((fd1 = open(device2, O_RDONLY)) == -1) {
		printf("%s is not a vaild device\n", device2);
	}

	while (*mode_memory != QUIT) {
		usleep(40000);
		read(fd1, &push_switch, sizeof(push_switch)); // push swtich 받아오기

		for (i = 0; i < 9; i++) {
			if (push_switch[i] != 0)
				flag = 1;
		}
		
		
		if(flag == 1) {
			for (i = 0; i < 9; i++) {
				input_memory[i] = push_switch[i];
				//printf("%d        ", input_memory[i]);
			}
			//printf("\n");
			//flag = 0;
		}
		if (*mode_memory == TEXTEDITOR_MODE) { // for double touch
			input_memory[9] = input_memory[1] * input_memory[2];
			input_memory[10] = input_memory[4] * input_memory[5];
			input_memory[11] = input_memory[7] * input_memory[8];
		}
		
		
	}
	close(fd1);
}

int output_process() {

	while (*mode_memory != QUIT) {
		switch (*mode_memory) {
			case QUIT:
				return;
			case DEFAULT_MODE:
				Default_mode_output();
				break;
			case CLOCK_MODE:
				Clock_mode_output();
				break;
			case COUNTER_MODE:
				Counter_mode_output();
				break;
			case TEXTEDITOR_MODE:
				Text_editor_mode_output();
				break;
			case DRAWBOARD_MODE:
				Draw_board_mode_output();
				break;
			

		}
	}
}
int Default_mode_output() {
	
	int i;
	unsigned char retval;
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr = 0;
	int led, fnd,lcd,dot;
	int dot_size;
	char str[32];
	led = open("/dev/mem", O_RDWR | O_SYNC);
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led, FPGA_BASE_ADDRESS);
	led_addr = (unsigned char*)((void*)fpga_addr + LED_ADDR);
	printf("default output\n");

	fnd = open("/dev/fpga_fnd", O_RDWR);

	lcd = open("/dev/fpga_text_lcd", O_WRONLY);

	dot = open("/dev/fpga_dot", O_WRONLY);
	dot_size = sizeof(fpga_number[18]);
	for (i = 0; i < 32; i++)
		str[i] = ' ';
	while (*mode_memory == DEFAULT_MODE) {
		usleep(40000);
		retval = write(fnd, output_memory, 4);       //fnd 덮어쓰기

		*led_addr = 0; // led 불키는부분 1~255
		usleep(40000);
		write(dot, dot_mode[2], dot_size);
		usleep(40000);
		write(lcd,str , 32);

	}
	close(lcd);
	close(led);
	close(dot);
	close(fnd);
	return 0;
}
int Clock_mode() {
	int tmp = 0, tmp2, i;
	int ex_buffer[4];

	printf("clock mode\n");
	while (*mode_memory == CLOCK_MODE) {
		/*
		for (i = 0; i < 4; i++) {
			printf(" [%d,", input_memory[i]);
			printf("%d] ", ex_buffer[i]);
		}
		*/

		if (ex_buffer[0] == 1 && input_memory[0] == 0) { // change time
			if(output_memory[5] == 0)
				output_memory[5] = 1;

			else if (output_memory[5] == 1) {
				output_memory[4] = 128;
				output_memory[5] = 0;
			}
		}

		if (output_memory[5] == 1) {
			if (ex_buffer[1] == 1 && input_memory[1] == 0) { // reset
				init_memory();

			}
			else if (ex_buffer[2] == 1 && input_memory[2] == 0) { // 1hour
				tmp = output_memory[0] * 10 + output_memory[1];
				if (tmp == 23) {
					output_memory[0] = 0;
					output_memory[1] = 0;
				}
				else if (tmp % 10 == 9) {
					output_memory[0]++;
					output_memory[1] = 0;
				}
				else
					output_memory[1]++;
					

			}
			else if (ex_buffer[3] == 1 && input_memory[3] == 0) {// 1min

				tmp = output_memory[0] * 10 + output_memory[1];
				tmp2 = output_memory[2] * 10 + output_memory[3];
				printf("----------%d---------\n", tmp2);
				if (tmp2 == 59) {
					if (tmp == 23) {
						output_memory[0] = 0;
						output_memory[1] = 0;
					}
					else if(tmp%10 == 9){
						output_memory[0]++;
						output_memory[1] = 0;
					}
					output_memory[2] = 0;
					output_memory[3] = 0;
				}
				else if (tmp % 10 == 9) {
					output_memory[2]++;
					output_memory[3] = 0;
				}
				else {
					output_memory[3]++;
				}

			}
		}
		//cal
		

		for (i = 0; i < 4; i++) {
			ex_buffer[i] = input_memory[i];
		}

		usleep(50000);

	}
	return 0;
}

int Clock_mode_output() {
	
	int i;
	unsigned char retval;
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr = 0;
	char str[4] = { '0','0','0','0' };
	int led, fnd;
	printf("clock output\n");
	led = open("/dev/mem", O_RDWR | O_SYNC);
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led, FPGA_BASE_ADDRESS);
	led_addr = (unsigned char*)((void*)fpga_addr + LED_ADDR);


	fnd = open("/dev/fpga_fnd", O_RDWR);

	while (*mode_memory == CLOCK_MODE) {
		usleep(40000);
		retval = write(fnd, output_memory, 4);       //fnd 덮어쓰기
		if(output_memory[5] == 0) 
			*led_addr = output_memory[4]; // led 불키는부분 1~255
		else if (output_memory[5] == 1) {
			*led_addr = 16;
			sleep(1);
			*led_addr = 32;
			sleep(1);
		}

	}
	*led_addr = 0;
	write(fnd, str, 4);
	close(led);
	close(fnd);
	return 0;
}

int Counter_mode() {
	int tmp=0,tmp2, i;
	int ex_buffer[4];

	printf("counter mode\n");
	while (*mode_memory == COUNTER_MODE) {
		/*
		for (i = 0; i < 4; i++) {
			printf(" [%d,", input_memory[i]);
			printf("%d] ", ex_buffer[i]);
		}
		*/
		
		if (ex_buffer[0] == 1 && input_memory[0]==0 ) { // 진수변환
			printf("--------------%d---------------\n", output_memory[4]);
			if (output_memory[4] == 10) {
				output_memory[4] = 8;
				output_memory[5] = 32;

				tmp2 = tmp % 512;
				output_memory[1] = tmp2 / 64;
				tmp2 %= 64;
				output_memory[2] = tmp2 / 8;
				tmp2 %= 8;
				output_memory[3] = tmp2;
				
			}
			else if (output_memory[4] == 8) {
				output_memory[4] = 4;
				output_memory[5] = 16;

				tmp2 = tmp % 64;
				output_memory[1] = tmp2 / 16;
				tmp2 %= 16;
				output_memory[2] = tmp2 / 4;
				tmp2 %= 4;
				output_memory[3] = tmp2;
				
			}
			else if (output_memory[4] == 4) {
				output_memory[4] = 2;
				output_memory[5] = 128;

				tmp2 = tmp % 8;
				output_memory[1] = tmp2 / 4;
				tmp2 %= 4;
				output_memory[2] = tmp2 / 2;
				tmp2 %= 2;
				output_memory[3] = tmp2;
				
			}
			else if(output_memory[4] == 2) {
				output_memory[4] = 10;
				output_memory[5] = 64;
				
			}
			
		}
		
		
		if (ex_buffer[1] == 1 && input_memory[1] == 0) { // 백의자리 1증가
			tmp += output_memory[4] * output_memory[4];
			
		}
		else if(ex_buffer[2] == 1 && input_memory[2] == 0){ // 십의 자리
			tmp += output_memory[4];
			
		}
		else if(ex_buffer[3] == 1 && input_memory[3] == 0){// 일의 자리
			tmp += 1;
			
		}
		//cal
		tmp2 = tmp % (output_memory[4] * output_memory[4] * output_memory[4]);
		output_memory[1] = tmp2 / (output_memory[4] * output_memory[4]);
		tmp2 = tmp2 % (output_memory[4] * output_memory[4]);
		output_memory[2] = tmp2 / output_memory[4];
		tmp2 %= output_memory[4];
		output_memory[3] = tmp2;

		for (i = 0; i < 4; i++) {
			ex_buffer[i] = input_memory[i];
		}

		usleep(50000);
		
	}
	return 0;
}

int Counter_mode_output() {
	int i;
	unsigned char retval;
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr = 0;
	int led, fnd;
	char str[4] = { '0','0','0','0' };

	led = open("/dev/mem", O_RDWR | O_SYNC);
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led, FPGA_BASE_ADDRESS);
	led_addr = (unsigned char*)((void*)fpga_addr + LED_ADDR);
	
	
	fnd = open("/dev/fpga_fnd", O_RDWR);

	while (*mode_memory == COUNTER_MODE) {
		usleep(40000);
		retval = write(fnd, output_memory, 4);       //fnd 덮어쓰기
		*led_addr = output_memory[5]; // led 불키는부분 1~255

		
	}
	*led_addr = 0;
	write(fnd, str, 4);
	close(led);
	close(fnd);
	return 0 ;
	
}

int Text_editor_mode() {
	int tmp = 0, tmp2, i;
	int cnt = 0, s_cnt = 0;
	int ex_buffer[12];
	int flag = 0;
	char str[9];

	printf("text_editor mode\n");
	while (*mode_memory == TEXTEDITOR_MODE) {

		if (ex_buffer[9] == 1 && input_memory[9] == 1) { // press check
			flag = 1;
		}

		else 	if (ex_buffer[10] == 1 && input_memory[10] == 1) { // press check		
			flag = 1;
		}

		else if (ex_buffer[11] == 1 && input_memory[11] == 1) { // press check
			flag = 1;
		}
		else if (ex_buffer[9] == 1 && input_memory[9] == 0) { // clear
			cnt++;
			s_cnt = 0;
			for (i = 0; i < 9; i++) {
				str[i] = ' ';
				output_memory[i + 4] = ' ';
			}
		}
		else if (ex_buffer[10] == 1 && input_memory[10] == 0) { // chage type
			cnt++;
			if (output_memory[15] == 0)
				output_memory[15] = 1;
			else if (output_memory[15] == 1)
				output_memory[15] = 0;
		}
		else if (ex_buffer[11] == 1 && input_memory[11] == 0) { // space
			cnt++;
			output_memory[s_cnt] = ' ';
			s_cnt++;
		}
		else if (ex_buffer[0] == 1 && input_memory[0] == 0 && flag == 0) { // .QZ 1
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[0][0])
						str[s_cnt - 1] = txt_list[0][1];
					else if (str[s_cnt - 1] == txt_list[0][1])
						str[s_cnt - 1] = txt_list[0][2];
					else if (str[s_cnt - 1] == txt_list[0][2])
						str[s_cnt - 1] = txt_list[0][0];
					else {
						str[s_cnt] = txt_list[0][0];
						s_cnt++;
					}

				}
				else {
					str[s_cnt] = txt_list[0][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '1';
				s_cnt++;
			}
			
		}

		else if (ex_buffer[1] == 1 && input_memory[1] == 0 && flag == 0) { // ABC 2
			cnt++;

			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[1][0])
						str[s_cnt - 1] = txt_list[1][1];
					else if (str[s_cnt - 1] == txt_list[1][1])
						str[s_cnt - 1] = txt_list[1][2];
					else if (str[s_cnt - 1] == txt_list[1][2])
						str[s_cnt - 1] = txt_list[1][0];
					else {
						str[s_cnt] = txt_list[1][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[1][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '2';
				s_cnt++;
			}
			

		}
		else if (ex_buffer[2] == 1 && input_memory[2] == 0 && flag == 0) { // DEF 3
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[2][0])
						str[s_cnt - 1] = txt_list[2][1];
					else if (str[s_cnt - 1] == txt_list[2][1])
						str[s_cnt - 1] = txt_list[2][2];
					else if (str[s_cnt - 1] == txt_list[2][2])
						str[s_cnt - 1] = txt_list[2][0];
					else {
						str[s_cnt] = txt_list[2][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[2][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '3';
				s_cnt++;
			}
			

		}

		else if (ex_buffer[3] == 1 && input_memory[3] == 0 && flag == 0) {// GHI 4
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[3][0])
						str[s_cnt - 1] = txt_list[3][1];
					else if (str[s_cnt - 1] == txt_list[3][1])
						str[s_cnt - 1] = txt_list[3][2];
					else if (str[s_cnt - 1] == txt_list[3][2])
						str[s_cnt - 1] = txt_list[3][0];
					else {
						str[s_cnt] = txt_list[3][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[3][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '4';
				s_cnt++;
			}
			
		}

		else if (ex_buffer[4] == 1 && input_memory[4] == 0 && flag == 0) {// JKL 5
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[4][0])
						str[s_cnt - 1] = txt_list[4][1];
					else if (str[s_cnt - 1] == txt_list[4][1])
						str[s_cnt] = txt_list[4][2];
					else if (str[s_cnt - 1] == txt_list[4][2])
						str[s_cnt - 1] = txt_list[4][0];
					else {
						str[s_cnt] = txt_list[4][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[4][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '5';
				s_cnt++;
			}
			
		}

		else if (ex_buffer[5] == 1 && input_memory[5] == 0 && flag == 0) {// MNO 6
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[5][0])
						str[s_cnt - 1] = txt_list[5][1];
					else if (str[s_cnt - 1] == txt_list[5][1])
						str[s_cnt - 1] = txt_list[5][2];
					else if (str[s_cnt - 1] == txt_list[5][2])
						str[s_cnt - 1] = txt_list[5][0];
					else {
						str[s_cnt] = txt_list[5][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[5][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '6';
				s_cnt++;
			}
			
		}

		else if (ex_buffer[6] == 1 && input_memory[6] == 0 && flag == 0) {// PRS 7
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[6][0])
						str[s_cnt - 1] = txt_list[6][1];
					else if (str[s_cnt - 1] == txt_list[6][1])
						str[s_cnt - 1] = txt_list[6][2];
					else if (str[s_cnt - 1] == txt_list[6][2])
						str[s_cnt - 1] = txt_list[6][0];
					else {
						str[s_cnt] = txt_list[6][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[6][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '7';
				s_cnt++;
			}
			
		}

		else if (ex_buffer[7] == 1 && input_memory[7] == 0 && flag == 0) {// TUV 8
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[7][0])
						str[s_cnt - 1] = txt_list[7][1];
					else if (str[s_cnt - 1] == txt_list[7][1])
						str[s_cnt - 1] = txt_list[7][2];
					else if (str[s_cnt - 1] == txt_list[7][2])
						str[s_cnt - 1] = txt_list[7][0];
					else {
						str[s_cnt] = txt_list[7][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[7][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '8';
				s_cnt++;
			}
			
		}

		else if (ex_buffer[8] == 1 && input_memory[8] == 0 && flag == 0) {// WXY 9
			cnt++;
			if (output_memory[15] == 0) { // eng
				if (s_cnt > 0) {
					if (str[s_cnt - 1] == txt_list[8][0])
						str[s_cnt - 1] = txt_list[8][1];
					else if (str[s_cnt - 1] == txt_list[8][1])
						str[s_cnt - 1] = txt_list[8][2];
					else if (str[s_cnt - 1] == txt_list[8][2])
						str[s_cnt - 1] = txt_list[8][0];
					else {
						str[s_cnt] = txt_list[8][0];
						s_cnt++;
					}
				}
				else {
					str[s_cnt] = txt_list[8][0];
					s_cnt++;
				}
			}
			else { //num
				str[s_cnt] = '9';
				s_cnt++;
			}
			
		}

		//lcd

		if (s_cnt == 9) {
			for (i = 1; i < 9; i++) {
				str[i - 1] = str[i];
			}
			
			s_cnt--;
		}
		for (i = 0; i < s_cnt; i++) {
			output_memory[i + 4] = str[i];
		}
		output_memory[16] = s_cnt;
		//fnd 
		tmp = cnt % 10000;
		output_memory[0] = tmp / 1000;
		tmp %= 1000;
		output_memory[1] = tmp / 100;
		tmp %= 100;
		output_memory[2] = tmp / 10;
		tmp %= 10;
		output_memory[3] = tmp;
		
		for (i = 0; i < 12; i++) {
			ex_buffer[i] = input_memory[i];
		}
		for (i = 0; i < 9; i++) {
			if (input_memory[i] == 1) break;
			else if (i == 8 && input_memory[i] == 0)
				flag = 0;
		}
		/*
		for (i = 0; i < 8; i++) {
			printf("%c      ",output_memory[i+4]);
		}
		printf("%d    %d \n",s_cnt,flag);
		*/
		usleep(50000);

	}
	return 0;
}

int Text_editor_mode_output() {
	int i;
	unsigned char retval;
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr = 0;
	int led, fnd,dot,lcd;
	int dot_size;
	char str_fnd[4] = { '0','0','0','0' };
	char str_lcd[32];
	for (i = 0; i < 32; i++)
		str_lcd[i] = ' ';
	led = open("/dev/mem", O_RDWR | O_SYNC);
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led, FPGA_BASE_ADDRESS);
	led_addr = (unsigned char*)((void*)fpga_addr + LED_ADDR);

	dot = open("/dev/fpga_dot", O_WRONLY);
	dot_size = sizeof(fpga_number[18]);

	lcd = open("/dev/fpga_text_lcd", O_WRONLY);
	fnd = open("/dev/fpga_fnd", O_RDWR);

	while (*mode_memory == TEXTEDITOR_MODE) {
		usleep(40000);
		retval = write(fnd, output_memory, 4);       //fnd 덮어쓰기
		if (output_memory[15] == 0) { // alph mode
			write(dot, dot_mode[0], dot_size);
		}
		else if (output_memory[15] == 1) { // num mode
			write(dot, dot_mode[1], dot_size);
		}
		usleep(40000);
		write(lcd, output_memory+4, 8);
	}

	*led_addr = 0;
	write(fnd, str_fnd, 4); // fnd 끄기
	write(dot, dot_mode[2], dot_size); //dot 끄기
	write(lcd, str_lcd, 32); // lcd 끄기
	close(led);
	close(fnd);
	close(dot);
	close(lcd);
	return 0;

}

int Draw_board_mode() {
	int tmp = 1, tmp2, i, j;
	int cnt = 0;
	int ex_buffer[9];
	int dot_mem[7][10];
	int mouse[2] = { 0,0 }; //x,y
	char mem[7] = { 0x40,0x20,0x10,0x08,0x04,0x02,0x01 };
	for (i = 0; i < 7; i++) {
		for (j = 0; j < 10; j++) {
			dot_mem[i][j] = 0;
		}
	}
	printf("draw_board mode\n");
	while (*mode_memory == DRAWBOARD_MODE) {


		if (ex_buffer[0] == 1 && input_memory[0] == 0) { // reset
			cnt++;
			for (i = 0; i < 7; i++) {
				for (j = 0; j < 10; j++) {
					dot_mem[i][j] = 0;
				}
			}
			mouse[0] = 0;
			mouse[1] = 0;
			for (i = 0; i < 10; i++) {
				output_memory[i + 4] = 0x00;
			}
		}

		else if (ex_buffer[1] == 1 && input_memory[1] == 0) { // up
			cnt++;
			if (mouse[1] < 9)
				mouse[1]++;


		}
		else if (ex_buffer[2] == 1 && input_memory[2] == 0) { // 1 : hide  
			cnt++;
			
			output_memory[15] = !output_memory[15];
		}

		else if (ex_buffer[3] == 1 && input_memory[3] == 0) {// left
			cnt++;
			if (mouse[0] > 0)
				mouse[0]--;

		}

		else if (ex_buffer[4] == 1 && input_memory[4] == 0) {// choose
			cnt++;
			dot_mem[mouse[0]][mouse[1]] = !dot_mem[mouse[0]][mouse[1]];
			if (dot_mem[mouse[0]][mouse[1]] == 1) {
				output_memory[13 - mouse[1]] += mem[mouse[0]];
			}
			else if (dot_mem[mouse[0]][mouse[1]] == 0) {
				output_memory[13 - mouse[1]] -= mem[mouse[0]];
			}
		}

		else if (ex_buffer[5] == 1 && input_memory[5] == 0) {// right
			cnt++;
			if (mouse[0] < 6)
				mouse[0]++;

		}

		else if (ex_buffer[6] == 1 && input_memory[6] == 0) {// clear
			cnt++;
			for (i = 0; i < 7; i++) {
				for (j = 0; j < 10; j++) {
					dot_mem[i][j] = 0;
				}
			}
			for (i = 0; i < 10; i++) {
				output_memory[i + 4] = 0x00;
			}
		}
	
		else if (ex_buffer[7] == 1 && input_memory[7] == 0) {// down
			cnt++;
			if (mouse[1] > 0)
				mouse[1]--;
		}

		else if (ex_buffer[8] == 1 && input_memory[8] == 0) {// reverse
			cnt++;
			for (i = 0; i < 7; i++) {
				for (j = 0; j < 10; j++) {
					dot_mem[i][j] = !dot_mem[i][j];
				}
			}
			for (i = 0; i < 10; i++) {
				output_memory[i+4] = 0x7F- output_memory[i+4];
			}
		}
		// for hide or not
		output_memory[18] = mem[mouse[0]];
		output_memory[19] = 13 - mouse[1];

		for (i = 0; i < 10; i++) {
			printf("%d     ", output_memory[i + 4]);
		}
		printf("\n");
		/*
		for (i = 0; i < 10; i++) {
			for (j = 0; j < 7; j++) {
				printf("%d    ", dot_mem[j][i]);

			}
			printf("\n");
		}
		*/
		
		
		tmp = cnt % 10000;
		output_memory[0] = tmp / 1000;
		tmp %= 1000;
		output_memory[1] = tmp / 100;
		tmp %= 100;
		output_memory[2] = tmp / 10;
		tmp %= 10;
		output_memory[3] = tmp;

		for (i = 0; i < 9; i++) {
			ex_buffer[i] = input_memory[i];
		}

		usleep(50000);

	}
	return 0;
}
int Draw_board_mode_output() {
	int i;
	unsigned char retval;
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr = 0;
	int led, fnd, dot, lcd;
	int dot_size;
	char tmp;


	led = open("/dev/mem", O_RDWR | O_SYNC);
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led, FPGA_BASE_ADDRESS);
	led_addr = (unsigned char*)((void*)fpga_addr + LED_ADDR);

	dot = open("/dev/fpga_dot", O_WRONLY);
	dot_size = sizeof(fpga_number[18]);

	fnd = open("/dev/fpga_fnd", O_RDWR);

	while (*mode_memory == DRAWBOARD_MODE) {
		usleep(40000);
		retval = write(fnd, output_memory, 4);       //fnd 덮어쓰기
		usleep(40000);
		/*
		if (output_memory[15] == 0) { // hide mode
			
			output_memory[output_memory[19]] += output_memory[18];
			write(dot, output_memory + 4, dot_size);
			sleep(1);
			output_memory[output_memory[19]] -= output_memory[18];
			write(dot, output_memory + 4, dot_size);
			sleep(1);
		}
		*/
		
			write(dot, output_memory + 4, dot_size);
		
			
		


	}

	close(led);
	close(fnd);
	close(dot);
	close(lcd);
	return 0;

}

int main(int argc, char *argv[]){
	pid_t pid;
	
	shared_memory();

	pid = fork();
	
	if (pid == -1) {
		printf("error\n");
		exit(0);
	}
	else if (pid == 0) {
		pid = fork();
			
		if (pid == -1)
			return 0;

		else if (pid == 0)
			return event0_process();

		else
			return input_process();
	}

	else {
		pid = fork();

		if (pid == -1) {
			printf("error\n");
			exit(0);
		}

		else if (pid == 0) {
			//output process
			//printf("child2 : getpid() %d, getppid() %d\n", getpid(), getppid());
			//printf("child2 : pid %d\n", pid2);
			//printf("child2 : glob %d var %d\n", glob, var);

			return output_process();
		}

		else {
			//main process
			//printf("parent : getpid() %d\n", getpid());
			//printf("parent : pid %d\n", pid2);
			//printf("parent : glob %d var %d\n", glob, var);
			return main_process();
		}
	}

	return 0;
}
