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
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Programs/misc.h"
#include "Programs/brl.h"

#define BRLNAME "Virtual"
#define PREFSTYLE ST_Generic

typedef enum {
  PARM_PORT
} DriverParameter;
#define BRLPARMS "port"

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "braille.h"

#define BRLROWS		1

static int fileDescriptor = -1;

#define INPUT_SIZE 0X200
static char inputBuffer[INPUT_SIZE];
static int inputLength;

static unsigned char *prevVisualData, *prevData; /* previously sent raw data */
static unsigned char prevStatus[StatusCellCount]; /* to hold previous status */
static int columns, statusCells = 0;

static int
acceptConnection (unsigned short port, unsigned long address) {
  int serverSocket = -1;
  int queueSocket;

  if ((queueSocket = socket(PF_INET, SOCK_STREAM, 0)) != -1) {
    struct sockaddr_in localAddress;

    /* lose the pesky "address already in use" error message */
    {
      int yes = 1;
      if (setsockopt(queueSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        LogError("setsockopt");
        /* non-fatal */
      }
    }

    memset(&localAddress, 0, sizeof(localAddress));
    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = address;
    localAddress.sin_port = htons(port);
    if (bind(queueSocket, (struct sockaddr *)&localAddress, sizeof(localAddress)) != -1) {
      if (listen(queueSocket, 1) != -1) {
        int attempts = 0;
        LogPrint(LOG_NOTICE, "Listening on %s:%u.",
                 inet_ntoa(localAddress.sin_addr),
                 ntohs(localAddress.sin_port));

        while (1) {
          fd_set readMask;
          struct timeval timeout;

          FD_ZERO(&readMask);
          FD_SET(queueSocket, &readMask);

          memset(&timeout, 0, sizeof(timeout));
          timeout.tv_sec = 10;

          ++attempts;
          switch (select(queueSocket+1, &readMask, NULL, NULL, &timeout)) {
            case -1:
              if (errno == EINTR) continue;
              LogError("select");
              break;

            case 0:
              LogPrint(LOG_DEBUG, "No connection yet, still waiting (%d).", attempts);
              continue;

            default: {
              struct sockaddr_in remoteAddress;
              int addressLength = sizeof(remoteAddress);

              if (!FD_ISSET(queueSocket, &readMask)) continue;

              if ((serverSocket = accept(queueSocket,
                                         (struct sockaddr *)&remoteAddress,
                                         &addressLength)) != -1) {
                LogPrint(LOG_NOTICE, "Client is %s:%u.",
                         inet_ntoa(remoteAddress.sin_addr),
                         ntohs(remoteAddress.sin_port));
              } else {
                LogError("accept");
              }
            }
          }
          break;
        }
      } else {
        LogError("listen");
      }
    } else {
      LogError("bind");
    }

    close(queueSocket);
  } else {
    LogError("socket");
  }

  return serverSocket;
}

static char *
makeString (const char *characters, int count) {
  char *string = mallocWrapper(count+1);
  memcpy(string, characters, count);
  string[count] = 0;
  return string;
}

static char *
copyString (const char *string) {
  return makeString(string, strlen(string));
}

static char *
readString (void) {
  fd_set readMask;
  struct timeval timeout;

  FD_ZERO(&readMask);
  FD_SET(fileDescriptor, &readMask);

  memset(&timeout, 0, sizeof(timeout));

  switch (select(fileDescriptor+1, &readMask, NULL, NULL, &timeout)) {
    case -1:
      LogError("select");
    case 0:
      break;
    
    default: {
      char *inputStart = &inputBuffer[inputLength];
      int received = recv(fileDescriptor, inputStart, INPUT_SIZE-inputLength, 0);

      if (received == -1) {
        LogError("recv");
        break;
      }

      if (received == 0) {
        LogPrint(LOG_NOTICE, "Remote closed connection.");
        return copyString("quit");
      }
      inputLength += received;

      while (received-- > 0) {
        if (*inputStart == '\n') {
          char *string;
          int stringLength = inputStart - inputBuffer;
          if ((inputStart != inputBuffer) && (*(inputStart-1) == '\r')) --stringLength;
          string = makeString(inputBuffer, stringLength);
          inputLength -= ++inputStart - inputBuffer;
          memmove(inputBuffer, inputStart, inputLength);
          return string;
        }

        ++inputStart;
      }
    }
  }

  return NULL;
}

/* Print the dots in buf in a humanly readable form into a string.
 * Caller is responsible for freeing string
 */
static char *
printDots (const unsigned char *cells, int count) {
  char *result = mallocWrapper((count*9)+1);
  char *p = result;

  while (count-- > 0) {
    unsigned char cell = *cells++;
    if (p != result) *p++ = '|';
    if (cell == 0) {
      *p++ = ' ';
    } else {
      if (cell & B1) *p++ = '1';
      if (cell & B2) *p++ = '2';
      if (cell & B3) *p++ = '3';
      if (cell & B4) *p++ = '4';
      if (cell & B5) *p++ = '5';
      if (cell & B6) *p++ = '6';
      if (cell & B7) *p++ = '7';
      if (cell & B8) *p++ = '8';
    }
  }
  *p = 0;

  return result;
}

static void
brl_identify (void) {
  LogPrint(LOG_NOTICE, "Virtual Braille Driver: version 0.1");
  LogPrint(LOG_INFO,   "  Copyright (C) 2003 by Mario Lang <mlang@delysid.org>");
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
  char *str = NULL;
  int port = -1;
  unsigned long addr = INADDR_ANY;

  if (*parameters[PARM_PORT]) {
    port = atoi(parameters[PARM_PORT]);
  } else {
    port = 9999;
  }

  if ((fileDescriptor = acceptConnection(port, addr)) <= 0)
    goto failure;

  inputLength = 0;
  while (!str || (strcmp(str, "quit") != 0)) {
    if (str)
      free(str);

    if ((str = readString())) {
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
  char *str = readString();

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
