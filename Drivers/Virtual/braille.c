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

typedef struct {
  const CommandEntry *entry;
  unsigned int maximum;
} CommandDescriptor;
static CommandDescriptor *commandDescriptors = NULL;
static const size_t commandSize = sizeof(*commandDescriptors);
static size_t commandCount;

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
      int received = read(fileDescriptor, inputStart, INPUT_SIZE-inputLength);

      if (received == -1) {
        LogError("read");
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

static size_t
getCommandCount (void) {
  size_t count = 0;
  const CommandEntry *entry = commandTable;
  while (entry++->name) ++count;
  return count;
}

static void
sortCommands (int (*compareCommands) (const void *command1, const void *command2)) {
  qsort(commandDescriptors, commandCount, commandSize, compareCommands);
}

static int
compareCommandCodes (const void *item1, const void *item2) {
  const CommandDescriptor *descriptor1 = item1;
  const CommandDescriptor *descriptor2 = item2;

  int code1 = descriptor1->entry->code;
  int code2 = descriptor2->entry->code;

  if (code1 < code2) return -1;
  if (code1 > code2) return 1;
  return 0;
}

static void
sortCommandCodes (void) {
  sortCommands(compareCommandCodes);
}

static int
compareCommandNames (const void *item1, const void *item2) {
  const CommandDescriptor *descriptor1 = item1;
  const CommandDescriptor *descriptor2 = item2;

  return strcmp(descriptor1->entry->name, descriptor2->entry->name);
}

static void
sortCommandNames (void) {
  sortCommands(compareCommandNames);
}

static void
allocateCommandDescriptors (void) {
  if (!commandDescriptors) {
    commandCount = getCommandCount();
    commandDescriptors = mallocWrapper(commandCount * commandSize);

    {
      CommandDescriptor *descriptor = commandDescriptors;
      const CommandEntry *entry = commandTable;
      while (entry->name) {
        descriptor->entry = entry++;
        descriptor->maximum = 0;
        ++descriptor;
      }
    }

    sortCommandCodes();
    {
      CommandDescriptor *descriptor = commandDescriptors + commandCount;
      int previousBlock = -1;
      while (descriptor-- != commandDescriptors) {
        int code = descriptor->entry->code;
        int currentBlock = code & VAL_BLK_MASK;
        if (currentBlock != previousBlock) {
          if (currentBlock) {
            descriptor->maximum = VAL_ARG_MASK - (code & VAL_ARG_MASK);
          }
          previousBlock = currentBlock;
        }
      }
    }

    sortCommandNames();
  }
}

static int
testCommandName (const void *key, const void *item) {
  const char *name = key;
  const CommandDescriptor *descriptor = item;
  return strcasecmp(name, descriptor->entry->name);
}

static const CommandDescriptor *
findCommand (const char *name) {
  return bsearch(name, commandDescriptors, commandCount, commandSize, testCommandName);
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

  allocateCommandDescriptors();
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

    write(fileDescriptor, (void *)outbuf, strlen(outbuf));

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
    write(fileDescriptor, (void *)buf, columns+10);
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
      write(fileDescriptor, printbuf, strlen(printbuf));
    } else {
      char *dots = printDots(st, statusCells);
      char outbuf[(statusCells*9)+10];

      snprintf(outbuf,sizeof(outbuf),"StCells=\"%s\"\n", dots);
      free(dots);

      write(fileDescriptor, outbuf, strlen(outbuf));
    }
  }
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext context) {
  int command = EOF;
  char *string = readString();

  if (string) {
    static const char *delimiters = " ";
    const char *word;
    LogPrint(LOG_DEBUG, "Command received: %s", string);

    if ((word = strtok(string, delimiters))) {
      if (strcasecmp(word, "quit") == 0) {
        command = CMD_RESTARTBRL;
      } else {
        const CommandDescriptor *descriptor = findCommand(word);
        if (descriptor) {
          int needsNumber = descriptor->maximum > 0;
          int numberSpecified = 0;
          int onSpecified = 0;
          int offSpecified = 0;
          int block;

          command = descriptor->entry->code;
          block = command & VAL_BLK_MASK;

          while ((word = strtok(NULL, delimiters))) {
            if (block == 0) {
              if (!onSpecified) {
                if (strcasecmp(word, "on") == 0) {
                  onSpecified = 1;
                  command |= VAL_SWITCHON;
                  continue;
                }
              }

              if (!offSpecified) {
                if (strcasecmp(word, "off") == 0) {
                  offSpecified = 1;
                  command |= VAL_SWITCHOFF;
                  continue;
                }
              }
            }

            if (needsNumber && !numberSpecified) {
              char *end;
              long int number = strtol(word, &end, 0);
              if (!*end) {
                numberSpecified = 1;
                if ((number > 0) && (number <= descriptor->maximum)) {
                  command += number;
                } else {
                  LogPrint(LOG_WARNING, "Number out of range: %s", word);
                }
                continue;
              }
            }

            LogPrint(LOG_WARNING, "Unknown option: %s", word);
          }

          if (needsNumber && !numberSpecified) {
            LogPrint(LOG_WARNING, "Number not specified.");
          }
        } else {
          LogPrint(LOG_WARNING, "Unknown command: %s", word);
        }
      }
    }

    free(string);
  }

  return command;
}
