/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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
#define VERSION "BRLTTY driver for Tty, version 0.1, 2004"
#define COPYRIGHT "Copyright Samuel Thibault <samuel.thibault@ens-lyon.org>"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include "Programs/brl.h"
#include "Programs/misc.h"
#include "Programs/scr.h"
#include "Programs/message.h"

#include <errno.h>
#include <curses.h>
#include <iconv.h>
#include <ctype.h>

typedef enum {
  PARM_TERM,
  PARM_LINES,
  PARM_COLS,
  PARM_CSET
} DriverParameter;
#define BRLPARMS "term", "lines", "cols", "cset"

#define BRL_HAVE_KEY_CODES
#include "Programs/brl_driver.h"
#include "braille.h"
#include "Programs/serial.h"

static int tty_fd=-1;
static iconv_t conv;
#define MAXLINES 3
#define MAXCOLS 80
#define WHOLESIZE (MAXLINES * MAXCOLS)
static int lines,cols;
static FILE *tty_ffd;
static SCREEN *scr;
static struct termios oldtio,newtio;

static void brl_identify()
{
 LogPrint(LOG_NOTICE, VERSION);
 LogPrint(LOG_INFO,   COPYRIGHT);
}

static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
 char *term="vt100";
 char *cset="ISO8859-1";

 if (*parameters[PARM_TERM])
  term=parameters[PARM_TERM];

 if (*parameters[PARM_CSET])
  cset=parameters[PARM_CSET];

 lines=1;
 if (*parameters[PARM_LINES]) {
  static const int minimum = 1;
  static const int maximum = MAXLINES;
  int value;
  if (validateInteger(&value, "lines", parameters[PARM_LINES], &minimum, &maximum))
   lines=value;
 }

 cols=40;
 if (*parameters[PARM_COLS]) {
  static const int minimum = 1;
  static const int maximum = MAXCOLS;
  int value;
  if (validateInteger(&value, "cols", parameters[PARM_COLS], &minimum, &maximum))
   cols=value;
 }

 if (!openSerialDevice(device, &tty_fd, &oldtio)) {
  LogPrint(LOG_ERR, "Tty open error: %s", strerror(errno));
  return 0;
 }
 memset(&newtio, 0, sizeof(newtio)); 
 newtio.c_cflag = CS8 | CLOCAL | CREAD;
 newtio.c_iflag = IGNPAR;
 newtio.c_oflag = 0;
 newtio.c_lflag = 0;
 resetSerialDevice(tty_fd,&newtio,B19200); 

 if (!(tty_ffd=fdopen(tty_fd,"a+"))) {
  LogError("fdopen");
  goto outfd;
 }

 if (!(scr=newterm(term,tty_ffd,tty_ffd))) {
  LogError("newterm");
  goto outffd;
 }

 cbreak();
 noecho();
 nonl();
 nodelay(stdscr, TRUE);
 intrflush(stdscr, FALSE);
 keypad(stdscr, TRUE);

 clear();
 refresh();
 fflush(tty_ffd);

 if ((conv=iconv_open(cset,"ISO8859-1"))==(iconv_t)-1) {
  LogPrint(LOG_ERR,"unable to open conversion");
  goto outcurses;
 }

 brl->x=cols;
 brl->y=lines; 

 LogPrint(LOG_INFO,"Tty: type=%s size=%dx%d",term,cols,lines);
 return 1;

 iconv_close(conv);
outcurses:
 endwin();
 delscreen(scr);
outffd:
 fclose(tty_ffd);
outfd:
 tcsetattr(tty_fd,TCSADRAIN,&oldtio);
 close(tty_fd);
 tty_fd=-1;
 return 0;
}

static void brl_close(BrailleDisplay *brl)
{
 if (tty_fd>=0)
 {
  iconv_close(conv);
  if (endwin()!=OK)
   LogPrint(LOG_ERR, "endwin error");
  delscreen(scr);
  fclose(tty_ffd);
  tcsetattr(tty_fd,TCSADRAIN,&oldtio);
  close(tty_fd);
  tty_fd=-1;
 }
}

static void brl_writeWindow(BrailleDisplay *brl)
{
 static unsigned char prevdata[WHOLESIZE];
 int i,j;
 char c,*pc,d,*pd;
 size_t sc,sd;
 static const unsigned char crlf[]="\r\n";
 if (memcmp(prevdata,brl->buffer,brl->x*brl->y)==0) return;
 clear();
 refresh();
 memcpy(&prevdata,brl->buffer,brl->x*brl->y);
 for (j=0; j<brl->y; j++) {
  for (i=0; i<brl->x; i++) {
   c = brl->buffer[j*brl->x+i];
   sc=1; sd=1;
   pc=&c; pd=&d;
   if (iconv(conv,&pc,&sc,&pd,&sd)<0 && !sd)
    write(tty_fd,&c,1);
   else
    write(tty_fd,&d,1);
  }
  write(tty_fd,&crlf,2);
 }
}

static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *s)
{
}

int brl_keyToCommand(BrailleDisplay *brl, DriverCommandContext caller, int code)
{
 switch (code) {
  case KEY_LEFT: return CMD_FWINLT;
  case KEY_RIGHT: return CMD_FWINRT;
  case KEY_UP: return CMD_LNUP;
  case KEY_DOWN: return CMD_LNDN;

  case KEY_PPAGE: return CMD_TOP;
  case KEY_NPAGE: return CMD_BOT;
  case KEY_HOME: return CMD_TOP_LEFT;
  case KEY_LL: return CMD_BOT_LEFT;
  case KEY_IC: return CMD_HOME;
  case KEY_DC: return CMD_CSRTRK;

  case KEY_F(1): return CMD_HELP;
  case KEY_F(2): return CMD_LEARN;
  case KEY_F(3): return CMD_INFO;
  case KEY_F(4): return CMD_PREFMENU;

  case KEY_F(5): return CMD_LNBEG;
  case KEY_F(6): return CMD_CHRLT;
  case KEY_F(7): return CMD_CHRRT;
  case KEY_F(8): return CMD_LNEND;

  case KEY_F(9): return CMD_PRPROMPT;
  case KEY_F(10): return CMD_NXPROMPT;
  case KEY_F(11): return CMD_PRPGRPH;
  case KEY_F(12): return CMD_NXPGRPH;

  case KEY_BACKSPACE: return VAL_PASSKEY | VPK_BACKSPACE;

  case EOF: return EOF;
  default:
   if (code <= 0XFF) return VAL_PASSCHAR | code;
   LogPrint(LOG_WARNING, "Unknown key: %02X", code);
   return CMD_NOOP;
 }
}

static int brl_readKey(BrailleDisplay *brl)
{
 int key;

 key=getch();
 if (key==ERR) {
  return EOF;
 }
 LogPrint(LOG_NOTICE,"key %d",key);
 return key;
}

static int brl_readCommand(BrailleDisplay *brl, DriverCommandContext context)
{
 int res;
 res=brl_keyToCommand(brl,context,brl_readKey(brl));
 if (res!=EOF)
  LogPrint(LOG_NOTICE,"=> %4x",res);
 return res;
}
