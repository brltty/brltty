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

#define BRL_C 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>                                                                  
#include <sys/socket.h>                                                                 

#include "brlconf.h"
#include "Programs/brltty.h"
#include "Programs/misc.h"
#include "Programs/brl_driver.h"

#define BRLROWS		1

/* Global variables */
static int fileDescriptor;
static unsigned char *prevVisualData, *prevData; /* previously sent raw data */
static unsigned char prevStatus[StatusCellCount]; /* to hold previous status */
static int columns, statusCells = 0;

int accept_connection(short port, unsigned long addr)                                                                     
{                                                                                  
  struct sockaddr_in myaddr, remoteaddr;
  int fd, newfd;
  int addrlen = sizeof(remoteaddr);
  short i;

  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {                      
    LogError("socket");                                                          
    return -1;                                                                   
  }                                                                              

  /* lose the pesky "address already in use" error message */
  {
    int yes=1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      LogError("setsockopt");                                                      
      return -1;                                                                   
    }                                                                              
  }

  myaddr.sin_family = AF_INET;                                                   
  myaddr.sin_addr.s_addr = addr;                                           
  myaddr.sin_port = htons(port);                                                 
  memset(&(myaddr.sin_zero), 0, 8);                                           

  if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {        
    LogError("bind");                                                            
    return -1;                                                                   
  }                                                                              

  if (listen(fd, 1) == -1) {                                              
    LogError("listen");                                                          
    return -1;                                                                   
  }                                                                              

  LogPrint(LOG_NOTICE, "Listening on %s:%d",
	   inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port));

                                                                                        
  for(i = 1; i<11; i++) {                                                                      
    int rc;
    fd_set read_fds;
    struct timeval tv = { 10, 0 };

    FD_ZERO(&read_fds);                                                            
    FD_SET(fd, &read_fds);                                                     

    if ((rc = select(fd+1, &read_fds, NULL, NULL, &tv)) == -1) {                  
      LogError("select");                                                      
      return -1;                                                               
    }                                                                          

    if (rc == 0) {
      LogPrint(LOG_DEBUG, "No connection yet, still waiting (%d)", i);
      continue;
    }
    if (FD_ISSET(fd, &read_fds)) {
      if ((newfd = accept(fd, (struct sockaddr *)&remoteaddr, &addrlen))
	  == -1) {
	LogError("accept");
	return -1;
      } else {
	LogPrint(LOG_INFO, "Connection from %s",
		 inet_ntoa(remoteaddr.sin_addr));
	close(fd);
	return newfd;
      }
    }
  }

  close(fd);
  return -1;
}

#define BUFLEN 512

static char buf[BUFLEN];
static short bufstart = 0;

char *readString(int fd)
{
  fd_set readfds;
  struct timeval tv = { 0, 50*1000 };
  int rc;

  FD_ZERO(&readfds);
  FD_SET(fd,&readfds);

  if ((rc = select(fd+1, &readfds, NULL, NULL, &tv)) == -1) {
    LogError("select");
    return NULL;
  }

  if (rc == 0) {
    return NULL;
  } else {
    int rcvd;

    if ((rcvd = recv(fd, (void *)buf+bufstart, BUFLEN-bufstart, 0)) == 0) {
      char *quitsym = (char *)malloc(5);
      strcpy(quitsym,"quit");
      LogPrint(LOG_NOTICE, "Remote closed connection\n");
      return quitsym;
    } else {
      char *p;
      bufstart = rcvd;
      for (p = buf; p<buf+bufstart; p++) {
	if (*p == '\n' && *(p-1) == '\r') {
	  short size = p-buf-1;
	  char *result = (char *)malloc(size);

	  memcpy(result, buf, size);
	  *(result+size) = 0;

	  memmove(buf, p+1, rcvd-(size+1));
	  bufstart = rcvd-(size+2);

	  return result;
	}
      }
    }
  }

  return NULL;
}

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Virtual Driver, version 0.1");
  LogPrint(LOG_INFO, "  Copyright (C) 2003 by Mario Lang <mlang@delysid.org>");
}

/* Print the dots in buf in a humanly readable form into a string.
   Caller is responsible for freeing string */
char *printDots(unsigned char *buf, int len) {
  char *result = (char *)malloc((len*9)+1);
  char *p = result;
  int i;

  for (i=0; i<len; i++) {
    if (buf[i] & B1) *p++ = '1';
    if (buf[i] & B2) *p++ = '2';
    if (buf[i] & B3) *p++ = '3';
    if (buf[i] & B4) *p++ = '4';
    if (buf[i] & B5) *p++ = '5';
    if (buf[i] & B6) *p++ = '6';
    if (buf[i] & B7) *p++ = '7';
    if (buf[i] & B8) *p++ = '8';
    if (buf[i] == 0) *p++ = ' ';
    if (i < len-1) *p++ = '|';
  }
  *p = 0;

  return result;
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *dev) {
  char *str = NULL;
  int port = -1;
  unsigned long addr = INADDR_ANY;

  if (*parameters[0]) {
    port = atoi(parameters[0]);
  } else {
    port = 9999;
  }

  if ((fileDescriptor = accept_connection(port, addr)) <= 0)
    goto failure;
      
  while (!str || (strcmp(str, "quit") != 0)) {
    if (str)
      free(str);

    if ((str = readString(fileDescriptor))) {
      LogPrint(LOG_DEBUG, "Received command '%s'", str);

      if (!strncmp(str, "cells ", 6)) {
	if ((columns = atoi(str+6)) > 0) {
	  LogPrint(LOG_DEBUG, "Received valid cells command");
	  break;
	} else {
	  LogPrint(LOG_WARNING, "Illegal cells command");
	  columns = 0;
	  goto failure;
	}
      } else if (!strncmp(str, "stcells ", 8)) {
	statusCells = atoi(str+8);
	if (statusCells > 0 && statusCells < StatusCellCount) {
	  LogPrint(LOG_DEBUG, "Received valid stcells command");
	} else {
	  LogPrint(LOG_WARNING, "Illegal stcells command");
	  statusCells = 0;
	}
      }
    }
  }

  if (str && (strcmp(str, "quit") == 0)) {
    LogPrint(LOG_NOTICE, "Client requested quit");
    goto failure;
  }

  if (columns > 0) {
    /* Set model params... */
    brl->helpPage = 0;
    brl->x = columns;
    brl->y = BRLROWS;

    statusCells = statusCells ? statusCells : StatusCellCount;

    /* Allocate space for buffers */
    prevData = (unsigned char *) malloc (brl->x * brl->y);
    prevVisualData = (unsigned char *) malloc (brl->x * brl->y);
    if (!prevVisualData || !prevData) {
      LogPrint(LOG_ERR, "Can't allocate braille buffers");
      goto failure;
    }

    return 1;
  }

failure:
  if (prevVisualData) {
    free(prevVisualData);
    prevVisualData = NULL;
  }
  if (prevData) {
    free(prevData);
    prevData = NULL;
  }
  if (fileDescriptor) close(fileDescriptor);

  if (str) {
    free(str);
    str = NULL;
  }

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  free(prevVisualData);
  prevVisualData = NULL;

  free(prevData);
  prevData = NULL;

  columns = statusCells = 0;

  close(fileDescriptor);
  fileDescriptor = -1;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, prevData, columns) != 0) {
    int i;
    char *dots = printDots(brl->buffer, columns);
    char outbuf[(columns*9)+11];

    sprintf(outbuf,"Braille=\"%s\"\n", dots);
    free(dots);

    send(fileDescriptor, (void *)outbuf, strlen(outbuf), 0);

    for (i=0; i<columns; ++i)
      prevData[i] = brl->buffer[i];
  }
}

static void
brl_writeVisual (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, prevVisualData, columns) != 0) {
    int i;
    char buf[columns+10];

    strcpy(buf,"Visual=\"");
    for (i=0; i<columns; ++i) {
      buf[i+8] = (prevVisualData[i] = brl->buffer[i]);
    }
    buf[columns+8] = '"'; buf[columns+9] = '\n';
    send(fileDescriptor, (void *)buf, columns+10, 0);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st) {
  if (memcmp(st, prevStatus, statusCells) != 0) {
    int i;

    for (i=0; i<statusCells; i++) prevStatus[i] = st[i];

    if (*st == FSC_GENERIC) {
      char printbuf[1024];
      snprintf(printbuf,sizeof(printbuf),
	       "Row/col: %d,%d\n",
	       st[STAT_BRLROW], st[STAT_BRLCOL]);
      send(fileDescriptor, printbuf, strlen(printbuf), 0);
    } else {
      char *dots = printDots(st, statusCells);
      char outbuf[(statusCells*9)+10];

      snprintf(outbuf,sizeof(outbuf),"StCells=\"%s\"\n", dots);
      free(dots);

      send(fileDescriptor, outbuf, strlen(outbuf), 0);
    }
  }
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext context) {
  int rc = EOF;
  char *str = readString(fileDescriptor);

  if (str) {
    LogPrint(LOG_DEBUG, "cmd '%s' received", str);

    if (strcmp(str, "quit") == 0) {
      rc = CMD_RESTARTBRL; goto done;
    } else if (strcmp(str, "noop") == 0) {
      rc = CMD_NOOP; goto done;
    } else if (strcmp(str, "lnup") == 0) {
      rc = CMD_LNUP; goto done;
    } else if (strcmp(str, "lndn") == 0) {
      rc = CMD_LNDN; goto done;
    } else if (strcmp(str, "winup") == 0) {
      rc = CMD_WINUP; goto done;
    } else if (strcmp(str, "windn") == 0) {
      rc = CMD_WINDN; goto done;
    } else if (strcmp(str, "prdifln") == 0) {
      rc = CMD_PRDIFLN; goto done;
    } else if (strcmp(str, "nxdifln") == 0) {
      rc = CMD_NXDIFLN; goto done;
    } else if (strcmp(str, "attrup") == 0) {
      rc = CMD_ATTRUP; goto done;
    } else if (strcmp(str, "attrdn") == 0) {
      rc = CMD_ATTRDN; goto done;
    } else if (strcmp(str, "top") == 0) {
      rc = CMD_TOP; goto done;
    } else if (strcmp(str, "bot") == 0) {
      rc = CMD_BOT; goto done;
    } else if (strcmp(str, "top_left") == 0) {
      rc = CMD_TOP_LEFT; goto done;
    } else if (strcmp(str, "bot_left") == 0) {
      rc = CMD_BOT_LEFT; goto done;
    } else if (strcmp(str, "prpgrph") == 0) {
      rc = CMD_PRPGRPH; goto done;
    } else if (strcmp(str, "nxpgrph") == 0) {
      rc = CMD_NXPGRPH; goto done;
    } else if (strcmp(str, "prprompt") == 0) {
      rc = CMD_PRPROMPT; goto done;
    } else if (strcmp(str, "nxprompt") == 0) {
      rc = CMD_NXPROMPT; goto done;
    } else if (strcmp(str, "prsearch") == 0) {
      rc = CMD_PRSEARCH; goto done;
    } else if (strcmp(str, "nxsearch") == 0) {
      rc = CMD_NXSEARCH; goto done;
    } else if (strcmp(str, "chrlt") == 0) {
      rc = CMD_CHRLT; goto done;
    } else if (strcmp(str, "chrrt") == 0) {
      rc = CMD_CHRRT; goto done;
    } else if (strcmp(str, "hwinlt") == 0) {
      rc = CMD_HWINLT; goto done;
    } else if (strcmp(str, "hwinrt") == 0) {
      rc = CMD_HWINRT; goto done;
    } else if (strcmp(str, "fwinlt") == 0) {
      rc = CMD_FWINLT; goto done;
    } else if (strcmp(str, "fwinrt") == 0) {
      rc = CMD_FWINRT; goto done;
    } else if (strcmp(str, "fwinltskip") == 0) {
      rc = CMD_FWINLTSKIP; goto done;
    } else if (strcmp(str, "fwinrtskip") == 0) {
      rc = CMD_FWINRTSKIP; goto done;
    } else if (strcmp(str, "lnbeg") == 0) {
      rc = CMD_LNBEG; goto done;
    } else if (strcmp(str, "lnend") == 0) {
      rc = CMD_LNEND; goto done;
    } else if (strcmp(str, "home") == 0) {
      rc = CMD_HOME; goto done;
    } else if (strcmp(str, "back") == 0) {
      rc = CMD_BACK; goto done;
    } else if (strcmp(str, "freeze") == 0) {
      rc = CMD_FREEZE; goto done;
    } else if (strcmp(str, "dispmd") == 0) {
      rc = CMD_DISPMD; goto done;
    } else if (strcmp(str, "sixdots") == 0) {
      rc = CMD_SIXDOTS; goto done;
    } else if (strcmp(str, "slidewin") == 0) {
      rc = CMD_SLIDEWIN; goto done;
    } else if (strcmp(str, "skpidlns") == 0) {
      rc = CMD_SKPIDLNS; goto done;
    } else if (strcmp(str, "skpblnkwins") == 0) {
      rc = CMD_SKPBLNKWINS; goto done;
    } else if (strcmp(str, "csrvis") == 0) {
      rc = CMD_CSRVIS; goto done;
    } else if (strcmp(str, "csrhide") == 0) {
      rc = CMD_CSRHIDE; goto done;
    } else if (strcmp(str, "csrtrk") == 0) {
      rc = CMD_CSRTRK; goto done;
    } else if (strcmp(str, "csrsize") == 0) {
      rc = CMD_CSRSIZE; goto done;
    } else if (strcmp(str, "csrblink") == 0) {
      rc = CMD_CSRBLINK; goto done;
    } else if (strcmp(str, "attrvis") == 0) {
      rc = CMD_ATTRVIS; goto done;
    } else if (strcmp(str, "attrblink") == 0) {
      rc = CMD_ATTRBLINK; goto done;
    } else if (strcmp(str, "capblink") == 0) {
      rc = CMD_CAPBLINK; goto done;
    } else if (strcmp(str, "tunes") == 0) {
      rc = CMD_TUNES; goto done;
    } else if (strcmp(str, "help") == 0) {
      rc = CMD_HELP; goto done;
    } else if (strcmp(str, "info") == 0) {
      rc = CMD_INFO; goto done;
    } else if (strcmp(str, "learn") == 0) {
      rc = CMD_LEARN; goto done;
    } else if (strcmp(str, "prefmenu") == 0) {
      rc = CMD_PREFMENU; goto done;
    } else if (strcmp(str, "prefsave") == 0) {
      rc = CMD_PREFSAVE; goto done;
    } else if (strcmp(str, "prefload") == 0) {
      rc = CMD_PREFLOAD; goto done;
    } else if (strcmp(str, "menu_first_item") == 0) {
      rc = CMD_MENU_FIRST_ITEM; goto done;
    } else if (strcmp(str, "menu_last_item") == 0) {
      rc = CMD_MENU_LAST_ITEM; goto done;
    } else if (strcmp(str, "menu_prev_item") == 0) {
      rc = CMD_MENU_PREV_ITEM; goto done;
    } else if (strcmp(str, "menu_next_item") == 0) {
      rc = CMD_MENU_NEXT_ITEM; goto done;
    } else if (strcmp(str, "menu_prev_setting") == 0) {
      rc = CMD_MENU_PREV_SETTING; goto done;
    } else if (strcmp(str, "menu_next_setting") == 0) {
      rc = CMD_MENU_NEXT_SETTING; goto done;
    } else if (strcmp(str, "say_line") == 0) {
      rc = CMD_SAY_LINE; goto done;
    } else if (strcmp(str, "say_above") == 0) {
      rc = CMD_SAY_ABOVE; goto done;
    } else if (strcmp(str, "say_below") == 0) {
      rc = CMD_SAY_BELOW; goto done;
    } else if (strcmp(str, "mute") == 0) {
      rc = CMD_MUTE; goto done;
    } else if (strcmp(str, "spkhome") == 0) {
      rc = CMD_SPKHOME; goto done;
    } else if (strcmp(str, "switchvt_prev") == 0) {
      rc = CMD_SWITCHVT_PREV; goto done;
    } else if (strcmp(str, "switchvt_next") == 0) {
      rc = CMD_SWITCHVT_NEXT; goto done;
    } else if (strcmp(str, "csrjmp_vert") == 0) {
      rc = CMD_CSRJMP_VERT; goto done;
    } else if (strcmp(str, "restartspeech") == 0) {
      rc = CMD_RESTARTSPEECH; goto done;
    }
  }

 done:
  if (str) free(str);

  return rc;
}
