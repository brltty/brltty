/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This Driver was written as a project in the
 *   HTL W1, Abteilung Elektrotechnik, Wien - Österreich
 *   (Technical High School, Department for electrical engineering,
 *     Vienna, Austria)
 *  by
 *   Tibor Becker
 *   Michael Burger
 *   Herbert Gruber
 *   Heimo Schön
 * Teacher:
 *   August Hörandl <hoerandl@elina.htlw1.ac.at>
 *
 * papenmeier/serial.c - Braille display test program
 * 
 *  This program simulates a papenmeier screen 2d terminal
 *  Start it on a second pc (connected via a serial line)
 *    serial [serialdev]
 *  use # to quit program
 *
 *  keys to simulate terminal
 *   left column   F1, F2 ..F10, Shift-F1, ...Shift-F12
 *   routing keys  qwertzuiop  yxcvbnm,.-
 *   bottom keys   12345678 DOWN LEFT SPACE UP RIGHT  
 *
 * See the GNU Public license for details in the ../COPYING file
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <curses.h>
#include <signal.h>

#define _SCR_H
#include "brl.c"

brldim dummy_brldim;		// unused

static void finish(int sig);
static void error(char* txt);

struct {
  int key;			// curses keycode 
  int code;			// code to send
  char* txt;			// debug output 
} key_data[] = {
  { -1, -1, "??" },
  // left column
  // linke Spalte - wert 768 - 831, delta 3, 22 tasten
  { KEY_F(1),  768, "L1" },
  { KEY_F(2),  771, "L2" },
  { KEY_F(3),  774, "L3" },
  { KEY_F(4),  777, "L4" },
  { KEY_F(5),  780, "L5" },
  { KEY_F(6),  783, "L6" },
  { KEY_F(7),  786, "L7" },
  { KEY_F(8),  789, "L8" },
  { KEY_F(9),  792, "L9" },
  { KEY_F(10), 795, "L10" },
  { KEY_F(11), 798, "L11" },
  { KEY_F(12), 801, "L12" },
  { KEY_F(13), 804, "L13" },
  { KEY_F(14), 807, "L14" },
  { KEY_F(15), 810, "L15" },
  { KEY_F(16), 813, "L16" },
  { KEY_F(17), 816, "L17" },
  { KEY_F(18), 819, "L18" },
  { KEY_F(19), 822, "L19" },
  { KEY_F(20), 825, "L20" },
  { KEY_F(21), 828, "L21" },
  { KEY_F(22), 831, "L22" },

  // bottom line
  // untere Zeile - wert 3 - 39, delta 3, 13 tasten
  { '1', 3, "1" },
  { '2', 6, "2" },
  { '3', 9, "3" },
  { '7', 12, "7" },
  { KEY_DOWN, 27 , "DOWN" },
  { KEY_LEFT, 18, "LEFT" },
  { ' ', 21, "SPC" },
  { KEY_RIGHT, 24, "RIGHT" },
  { KEY_UP, 15, "UP" },
  { '4', 30, "4" },
  { '5', 33, "5" },
  { '6', 36, "6" },
  { '8', 39, "8" },

  // routing keys
  // Zeile  - wert 834 - 1071, delta 3, 80 tasten
  { 'q', 834, "POS1" },
  { 'w', 837, "POS2" },
  { 'e', 840, "POS3" },
  { 'r', 843, "POS4" },
  { 't', 846, "POS5" },
  { 'z', 849, "POS6" },
  { 'u', 852, "POS7" },
  { 'i', 855, "POS8" },
  { 'o', 858, "POS9" },
  { 'p', 861, "POS10" },
  { 'ü', 864, "POS11" },
  { '+', 867, "POS12" },

  { '<', 1041, "POS70" },
  { 'y', 1044, "POS71" },
  { 'x', 1047, "POS72" },
  { 'c', 1050, "POS73" },
  { 'v', 1053, "POS74" },
  { 'b', 1056, "POS75" },
  { 'n', 1059, "POS76" },
  { 'm', 1062, "POS77" },
  { ',', 1065, "POS78" },
  { '.', 1068, "POS79" },
  { '-', 1071, "POS80" },
};

const int max_data = sizeof(key_data)/sizeof(key_data[0]);

/* decode statuscell to value -- needs update */
int nibble(unsigned int i)
{
  switch (i) 
    {
    case 14: return 0;
    case 1: return 1;
    case 5: return 2;
    case 3: return 3;
    case 11: return 4;
    case 9: return 5;
    case 7: return 6;
    case 15: return 7;
    case 13: return 8;
    case 6: return 9;
    default:
      return 100*i;
    }
}

int byte(unsigned int i)
{
  return 10*nibble(i/16) + nibble(i%16); 
}

void show_status(WINDOW* status, unsigned char* statcells)
{
  unsigned char txt[200];

  sprintf(txt, "\nline: (0x%02x) %d csr: (0x%02x) %d / (0x%02x) %d\n"
	  "%s %s %s %s %s %s %s %02x/%d %s %s %s",
	  statcells[0], byte(statcells[0]), 
          statcells[2], byte(statcells[2]), 
          statcells[3], byte(statcells[3]),
	  statcells [6]  ? "TRK" : "   ",
	  statcells [7]  ? "DISP" : "   ",
	  statcells [9]  ? "FRZ" : "   ",
	  statcells [13] ? "VIS" : "   ",
	  statcells [14] ? "SIZ" : "   ",
	  statcells [15] ? "BLNK" : "    ",
	  statcells [16] ? "CAP" : "   ",
	  statcells[17], byte(statcells[17]),
	  statcells [18] ? "WIN" : "   ",
	  statcells [19] ? "SND" : "   ",
	  statcells [20] ? "SKIP" : "    ");
  waddstr(status, txt);
  wrefresh(status);
}

// search for key c in the key_data table
int search(int c)
{
  int i;
  for(i=1; i < max_data; i++)
    if (c == key_data[i].key)
      return i;
  return 0;
}


void send_serial(int keycode, int pressed)
{
  // simulate time tick
  static int timeval = 0;

  unsigned char puffer[10];

  puffer[0] = 2;		// STX - Begin
  puffer[1] = 'K';		// 'K' 
  puffer[2] = keycode / 256;	
  puffer[3] = keycode % 256;
  puffer[4] = 0;		// length
  puffer[5] = 10;
  puffer[6] = pressed;		// 1 - pressed, 0 - release
  puffer[7] = timeval / 256;
  puffer[8] = timeval % 256;
  puffer[9] = 3;		// ETX - End

  timeval++;  
  write(brl_fd, puffer, sizeof(puffer));
  
  /*
    puffer[6] = 0;
    timeval++;  
    write(brl_fd, puffer, sizeof(puffer));
  */
}

void delay (int msec)
{
  struct timeval del;

  del.tv_sec = 0;
  del.tv_usec = msec * 1000;
  select (0, NULL, NULL, NULL, &del);
}



int read_serial(WINDOW* zeile, WINDOW* debug, WINDOW* status)
{
  unsigned char buf [100];
  unsigned char txt [500];

  int bytes;
  int i, l, o;

  READ(0);
  if (buf[0] != 2) {
    sprintf(txt,"?%02x", buf[0]);
    waddstr(debug, txt);
    return 0;
  }
  delay (100);		/* sleep for a while - to get the whole frame */

  read(brl_fd,buf+1,1);
  read(brl_fd,buf+2,1);
  read(brl_fd,buf+3,1);
  read(brl_fd,buf+4,1);
  read(brl_fd,buf+5,1);

  o = 0x100*buf[2] + buf[3]; /* Buffer offset */
  l = 0x100*buf[4] + buf[5]; /* Data count */

  sprintf(txt,"got %02x %02x %02x %02x %02x %02x - o/l = %d/%d\n", 
	  buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], o, l);
  waddstr(debug, txt);

  if (l > sizeof(buf))
    {
      sprintf(txt, "overflow %d", l);
      waddstr(debug, txt);
    }
  else
    {
      //      waddstr(debug, "READ");
      for(i = 6; i < l; i++) 
	read(brl_fd,buf+i,1);
      //      waddstr(debug, "DONE");
  
      if (buf[l-1] != cETX) {		/* ETX - End */
	sprintf(txt, "data: ");
	for(i=6; i<l; i++)
	  sprintf(txt +  6 + 3*i - 18, " %02x", buf[i]);
	waddstr(debug, txt);
      }

      if (o == offsetHorizontal && l == 87)
	{
	  sprintf(txt, "\n%80.80s",buf + 6);
	  waddstr(zeile, txt);
	  wrefresh(zeile);
	}
      else if (o == offsetVertical && l == 29)
	show_status(status, buf+6);
      else
	{
	  sprintf(txt, "\nunknown offset/length ", o);
	  waddstr(debug, txt);
	}
    }
  wrefresh(debug);
  wrefresh(zeile);
  wrefresh(status);
  return 0;
}

int main(int argc, char* argv[])
{
  int height, width;
  int h4;
  int c;
  WINDOW * zeile;
  WINDOW * status;
  WINDOW * debug_key;
  WINDOW * debug_serial;

  signal(SIGINT, finish);

  initscr(); cbreak(); noecho();

  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  scrollok(stdscr, TRUE);

  getmaxyx(stdscr, height, width);

  h4 = height/4;

  if (h4 < 3 || (h4-1)*width < 2*BRLCOLS)
    error("Display to small");

  addstr("---- Braille Display ----\n");
  mvaddstr(h4, 0, "---- Status ----\n");
  mvaddstr(2*h4, 0, "---- Keyboard ---- 7531 DWN LEFT SPACE RIGHT UP 2468\n");
  mvaddstr(3*h4, 0,  "---- Serial Debug ----\n");
  refresh();

  zeile        = subwin(stdscr, h4-1, 0, 1, 0);
  status       = subwin(stdscr, h4-1, 0, h4+1, 0);
  debug_key    = subwin(stdscr, h4-1, 0, 2*h4+1, 0);
  debug_serial = subwin(stdscr, h4-1, 0, 3*h4+1, 0);

  //debug_serial = debug_key;
  if (! (zeile && status && debug_key && debug_serial)) 
    error("OOPS - cant open windows");

  keypad(debug_key, TRUE);
  scrollok(zeile, TRUE);
  scrollok(status, TRUE);
  scrollok(debug_key, TRUE);
  scrollok(debug_serial, TRUE);

  // open serial
  dummy_brldim = initbrl(argv[1]);

  if (! brl_fd)
    error("OOPS - cant open " BRLDEV);

  c = ' ';
  do {
      char buffer[11];
      fd_set rfds;
      int retval;
      int i;

      /* Watch stdin (fd 0) and brl_fd to see when it has input. */
      FD_ZERO(&rfds);
      FD_SET(0, &rfds); // Tastatur
      FD_SET(brl_fd, &rfds);

      retval = select(brl_fd+1, &rfds, NULL, NULL, NULL);

      if (retval < 0) {
	perror("select");
	c = '#';
      } else 
	if (FD_ISSET(0, &rfds)) { // key pressed
	  c = wgetch(debug_key);
	  
	  i = search(c);      
	  sprintf(buffer, "%s ", key_data[i].txt);
	  waddstr(debug_key, buffer);
	  if (i)
	    send_serial(key_data[i].code,1); // key pressed
	  else
	    send_serial(0,0); // dummy key release 

	  wrefresh(debug_key);
	} 
        if (FD_ISSET(brl_fd, &rfds)) // data from brl_fd
	  read_serial(zeile, debug_serial, status);
	wrefresh(debug_serial);
	wrefresh(zeile);
	wrefresh(status);
	wrefresh(debug_key);
    } while (c != '#');
  
  finish(0);               /* we're done */
}

static void finish(int sig)
{
  endwin();
  closebrl (dummy_brldim);
  exit(sig);
}

static void error(char* txt)
{
  puts(txt);
  finish(1);
}
