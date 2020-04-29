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
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1

#define KEY_BACK 158
#define KEY_VOLUP 115
#define KEY_VOLDOWN 114

#define DEFAULT_MODE 1
#define CLOCK_MODE 2
#define COUNTER_MODE 3
#define TEXTEDITOR_MODE 4
#define DRAWBOARD_MODE 5



int mode_num;
key_t mode_key, input_key, output_key;
int mode_memory_id, input_memory_id, output_memory_id;
char *input_memory, *output_memory;
int *mode_memory;
int shared_memory() {
	printf("shared_memory\n");
	// shared_memory »ý¼º
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
	

	printf("init_memory\n");
	return 0;
}



int main_process(void) {
	printf("main\n");
	while (1) {
	}
}

int input_process() {
	struct input_event ev[BUFF_SIZE];
	int fd, rd, size = sizeof(struct input_event);
	int fd1, rd1;
	int i;
	int cnt = 1;
	unsigned char push_switch[9];

	char* device = "/dev/input/event0";
	if ((fd = open(device, O_RDONLY)) == -1) { 
		printf("%s is not a vaild device\n", device);
	}
	char* device2 = "/dev/fpga_push_switch";
	if ((fd1 = open(device2, O_RDONLY)) == -1) {
		printf("%s is not a vaild device\n", device2);
	}
	
	
	while (1) {
		if ((rd = read(fd, ev, size * BUFF_SIZE)) < size)
		{
			printf("read()");
			return (0);
		}


		if (ev[0].value != ' ' && ev[1].value == 1 && ev[1].type == 1) { // Only read the key press event
			printf("code%d\n", (ev[1].code));
		}

		if (ev[0].value == KEY_RELEASE) { // volup btn
			if (ev[0].code == KEY_VOLUP) {
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
			
			read(fd1, &push_switch, sizeof(push_switch));
			for (i = 0; i < 9; i++) {
				printf("%d        ", push_switch[i]);
			}


			printf("\n");
			switch (*mode_memory) {
			case CLOCK_MODE:
				Clock_mode();
				break;

			case COUNTER_MODE:
				Counter_mode();
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

	return 0;
}

int output_process() {
	printf("output\n");
	while (1);
}

int Clock_mode() {
	printf("clock mode\n");
}

int Counter_mode() {
	printf("counter mode\n");

}

int Text_editor_mode() {
	printf("text_editor mode\n");
}

int Draw_board_mode() {
	printf("draw board mode \n");
}

int main(int argc, char *argv[]){
	pid_t pid1, pid2;
	
	shared_memory();

	pid1 = fork();

	if (pid1 == -1) {
		printf("error\n");
		exit(0);
	}
	else if (pid1 == 0) {
		//input process
		//printf("child1 : getpid() %d, getppid() %d\n", getpid(), getppid());
		//printf("child1 : pid %d\n", pid1);
		//printf("child1 : glob %d var %d\n", glob, var);
		return input_process();
	}

	else {
		pid2 = fork();

		if (pid2 == -1) {
			printf("error\n");
			exit(0);
		}

		else if (pid2 == 0) {
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
