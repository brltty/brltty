/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

/*   August Hörandl <august.hoerandl@gmx.at>
 *
 * this programm is used to get the keycodes from an unknown terminal
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <curses.h>
#include <signal.h>
#include <termios.h>
#include <string.h>

int brl_fd = 0;			/* file descriptor for Braille display */
struct termios oldtio;		/* old terminal settings */

void try_init(const char *dev, unsigned int baud)
{
  struct termios newtio;	/* new terminal settings */

  printf("opening %s\n", dev);

  /* Now open the Braille display device for random access */

  brl_fd = open (dev, O_RDWR | O_NOCTTY);
  if (brl_fd < 0) {
    perror("open failed");
    exit(99);;
  }

  tcgetattr (brl_fd, &oldtio);	/* save current settings */

  /* Set bps, flow control and 8n1, enable reading */

  newtio.c_cflag = baud | CRTSCTS | CS8 | CLOCAL | CREAD;

  /* Ignore bytes with parity errors and make terminal raw and dumb */
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;		/* raw output */
  newtio.c_lflag = 0;		/* don't echo or generate signals */
  newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
  newtio.c_cc[VTIME] = 0;
  tcflush (brl_fd, TCIFLUSH);	/* clean line */
  tcsetattr (brl_fd, TCSANOW, &newtio);		/* activate new settings */

  return;
}


int main(int argc, char* argv[])
{
      fd_set rfds;
      int retval;
      struct timeval tv;

      try_init(argv[1], B19200);
      /* try_init(argv[1], B38400); */

      while (1) {
	/* Wait up to 10 seconds. */
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	/* Watch  brl_fd to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(brl_fd, &rfds);
	retval = select(brl_fd+1, &rfds, NULL, NULL, &tv);
	if (retval < 0) {
	  perror("select");
	  break;
	} else 
	  if (FD_ISSET(brl_fd, &rfds)) { /* data from brl_fd */
	    char c;
	    read(brl_fd,&c,1);
	    printf("%02x ", c);
	    if (c == 3)
	      printf("\n");
	  } else
	    break;
      }

     close(brl_fd);
}
