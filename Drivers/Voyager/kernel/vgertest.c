/* Program to test the driver */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <linux/types.h>

#include "linux/brlvger.h"

void read_display_keys(int fd);
void display_formatted_braille_input(char *old, char *new);
void set_keypress(void);
void reset_keypress(void);
void echo_on(void);
void echo_off(void);
int wait_keyboard_input(void);

static struct termios stored_settings;

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

const int braille_key[] = {4, 3, 2, 5, 6, 7, 1, 8};

const char* function_key[] = {	
	"Leftmost", 
	"Left", 
	"Left Arrow", 
	"Up Arrow", 
	"Down Arrow", 
	"Right Arrow", 
	"Right", 
	"Rightmost"};


void echo_off(void)
{
	struct termios new_settings;
	tcgetattr(0,&stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= (~ECHO);
	tcsetattr(0,TCSANOW,&new_settings);
	return;
}

void echo_on(void)
{
	tcsetattr(0,TCSANOW,&stored_settings);
	return;
}

void set_keypress(void)
{
	struct termios new_settings;

	tcgetattr(0,&stored_settings);

	new_settings = stored_settings;

	new_settings.c_lflag &= (~ICANON);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;

	tcsetattr(0,TCSANOW,&new_settings);
	return;
}

void reset_keypress(void)
{
	tcsetattr(0,TCSANOW,&stored_settings);
	return;
}

int wait_keyboard_input(void)
{
	int fdin=fileno(stdin), keyb_key;
	fd_set input;
	FD_ZERO(&input);
	FD_SET(fdin, &input);
	select(fdin+1, &input, NULL, NULL, NULL);
	if(FD_ISSET(fdin, &input))
	{
		read(fdin, &keyb_key, 1);
		return(keyb_key);
	}
	return (-1);
}

void display_test_patterns(int fd)
{
	char buf[44];
	int i;

	memset(buf, 0xff, sizeof(buf));

	fprintf(stdout, "Press any key to start\n");

	wait_keyboard_input();

	for(i=0;i<44;i+=4)
	{
		lseek(fd, i, SEEK_SET);
		write(fd, buf, 4);
		wait_keyboard_input();
	}
	
	lseek(fd, 10, SEEK_SET);
	memset(buf, 0x0, 44);
	write(fd, buf, 10);
	wait_keyboard_input();
}

void read_display_keys(int fd)
{
	int keyb_key;
	char buf[2][8];
	int new=0, old=1, res, fdin = fileno(stdin), duration=1000;
	fd_set input;

	fprintf(stdout, "\nPress 'q' to exit...\n");
	fprintf(stdout, "\nRead loop...\n");
	memset(buf,'\0', sizeof(buf));

	for(;;)
	{
		FD_ZERO(&input);
		FD_SET(fdin, &input);
		FD_SET(fd,&input);
		res = select(MAX(fd, fdin)+1, &input, NULL, NULL, NULL);

		if(res == -1)
		{
			perror("select");
			exit(-1);
		}

		if(FD_ISSET(fd, &input))
			if( (res = read(fd, buf[new], 8)) != -1)
			{
				display_formatted_braille_input(buf[old], buf[new]);
				new = (new)?0:1;
				old = (old)?0:1;
			}
			else
			{
				perror("braille read");
				exit(-1);
			}
		else 
		{
			read(fdin, &keyb_key, 1);
			if(keyb_key == 'b')
				ioctl(fd, BRLVGER_BUZZ, &duration);
			else if(keyb_key == 'q')
				break;
		}
	}
}

void display_formatted_braille_input(char *old, char *new)
{
	int idx, i, j;
	volatile unsigned char key_mask = 0;


	if(!old || !new)
		return;

	key_mask = old[0] ^ new[0];	

	while((idx = ffs(key_mask)) != 0)
	{
		int pressed = (1<<idx-1) & new[0];	
		fprintf(stdout, "Braille key %d %s\n", 
				braille_key[idx-1], pressed ? "pressed" : "released");
		key_mask ^= (1<<idx-1);
	}

	key_mask = old[1] ^ new[1];

	while((idx = ffs(key_mask)) != 0)
	{
		int pressed = (1<<idx-1) & new[1];	
		fprintf(stdout, "%s %s\n", 
				function_key[idx-1], pressed ? "pressed" : "released");
		key_mask ^= (1<<idx-1);
	}

	for(i=2;i<8;i++)
	{
		int found=0;

		if(old[i] == 0)
			break;

		for(j=2;j<8;j++)
			if(old[i] == new[j])
			{
				found=1;
				break;
			}
		if(found)
			continue;
		else
			fprintf(stdout, "Routing key %d released\n", old[i]);
	}

	for(i=2;i<8;i++)
	{
		int found=0;

		if(new[i] == 0)
			break;

		for(j=2;j<8;j++)
			if(new[i] == old[j])
			{
				found=1;
				break;
			}
		if(found)
			continue;
		else
			fprintf(stdout, "Routing key %d pressed\n", new[i]);
	}
}

void cleanup()
{
	reset_keypress();
	echo_on();
}


int main(int argc, char *argv[])
{
	char clear[44];
	int fd;
	int beep = 1000;
	int on_off;

	set_keypress();
	echo_off();
	atexit(cleanup);

	if( argc != 2 )
	{
		printf( "Usage: %s device_name\n", argv[0] );
		exit( -1 );
	}

	/* open display */
	if( (fd = open( argv[1], O_RDWR )) == -1 )
	{
		perror( "open: ");
		exit( -1 );
	}

	/* display on */
	if( ioctl(fd, BRLVGER_DISPLAY_ON, NULL) == -1 )
	{
		perror( "ioctl: ");
		exit( -1 );
	}

	fprintf(stdout,"\nBraille display on...\n");

#if 0
	{
	  int i,j;
	  __u16 voltage;

	  sleep(15);
	  for(i=160; i<=220; i+=10) {
	    voltage = i;
	    if(ioctl(fd, BRLVGER_SET_VOLTAGE, &voltage) <0) {
	      perror("ioctl BRLVGER_SET_VOLTAGE"); exit(1);
	    }

	    printf("%3d: ", i);
	    for(j=0; j<20; j++) {
	    again:
	      voltage = 0;
	      if(ioctl(fd, BRLVGER_GET_VOLTAGE, &voltage) <0) {
		perror("ioctl BRLVGER_GET_VOLTAGE"); goto again;
	      }
	      sleep(1);
	      printf("%3u ", voltage);
	      fflush(NULL);
	    }
	    printf("\n");
	  }
	}
#endif


	/* clear display */
	memset(clear, 0, sizeof(clear));
	lseek(fd, 0, SEEK_SET);
	write(fd, clear, sizeof(clear));

	fprintf(stdout,"\nDisplay cleared...\n");

	fprintf(stdout, "\nTesting display with patterns\n");

	display_test_patterns(fd);

	read_display_keys(fd);

	fprintf(stdout, "\nBye!\n\n");

	/* display off */
	if( ioctl(fd, BRLVGER_DISPLAY_ON, NULL) == -1 )
	{
		perror( "ioctl: ");
		exit( -1 );
	}

	fprintf(stdout,"\nBraille display off...\n");

	close(fd);
}

