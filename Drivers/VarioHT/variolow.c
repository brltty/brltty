/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include "variolow.h"
#include "Programs/misc.h"


	/*	The filedescriptor of the open port, sorry to say, wee need a global
	 *	one here */ 
static int devfd=0;

int varioinit(char *dev) 
{
	struct 	termios		tiodata;

		/*	Idiot check one */ 
	if(!dev) {
		return -1;
	}
	devfd=open(dev,O_RDWR | O_NOCTTY);
		/*	If we got it open, get the old attributed of the port */ 
	if(devfd==-1 || tcgetattr(devfd, &tiodata)) {
		LogPrint(LOG_ERR,"Port open failed: %s: %s",dev,strerror(errno));
		if(devfd>0) {
			close(devfd);
		}
		return -1;
	}
	tiodata.c_cflag=(CLOCAL|PARODD|PARENB|CREAD|CS8);
	tiodata.c_iflag=IGNPAR;
	tiodata.c_oflag=0;
	tiodata.c_lflag=0;
	tiodata.c_cc[VMIN]=0;
	tiodata.c_cc[VTIME]=0;
	
		/*	Force down DTR, flush any pending data and then 
		 *	the port to what we want it to be */ 
	if(
		cfsetispeed(&tiodata,B0) ||
		cfsetospeed(&tiodata,B0) ||
		tcsetattr(devfd,TCSANOW,&tiodata) ||
		tcflush(devfd, TCIOFLUSH) ||
		cfsetispeed(&tiodata,B19200) ||
		cfsetospeed(&tiodata,B19200) ||
		tcsetattr(devfd,TCSANOW,&tiodata) 
	  ) {
		LogPrint(LOG_ERR,"Port init failed: %s: %s",dev,strerror(errno));
		return -1;
	}
		/*	Pause to let them take effect */ 
	usleep(500);	
	return 0;
}

int varioclose()
{
		/*	Flush all pending output and then close the port */
	if(devfd>0) {
		tcdrain(devfd);
		close(devfd);
		return 0;
	}
	return -1;
}

int varioreset() 
{
	char c=VARIO_RESET;
	
	if(devfd<=0) {
		return -1;
	}
	return write(devfd,&c,sizeof(c))!=1;
}

int variodisplay(char *buff)
{

	if(!buff) {
		return -1;
	}
		/*	Write header and then data ... */ 
	write(devfd,VARIO_DISPLAY_DATA,VARIO_DISPLAY_DATA_LEN);
	write(devfd,buff,40);
	
	return 0;
}

int variocheckwaiting() 
{
	fd_set			checkset;
	struct	timeval	tval;

	tval.tv_sec=0;
	tval.tv_usec=0;

	FD_ZERO(&checkset);
	FD_SET(devfd,&checkset);
	return !select(devfd+1,&checkset,NULL,NULL,&tval);
}

int varioget()
{
	unsigned char 	data=0;
		/*	And read a tiny little byte */ 	
	if(read(devfd,&data,1)==1) {
		return data;
	}else {
		return -1;
	}
}

int variotranslate(char *frombuff, char *tobuff,int count) 
{
	if(!frombuff|!tobuff) {
		return -1;
	}
		/*	Just apply the nify macro for all chars */ 
	for(;count>=0;count--) {
		tobuff[count-1]=CHAR_TO_VARIO_CHAR(frombuff[count-1]);
	}

	return 0;
}
