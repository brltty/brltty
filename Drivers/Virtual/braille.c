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
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Programs/misc.h"
#include "Programs/brl.h"

#define BRLNAME "Virtual"
#define PREFSTYLE ST_Generic

typedef enum {
  PARM_SOCKET
} DriverParameter;
#define BRLPARMS "socket"

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "braille.h"

static int fileDescriptor = -1;

#define INPUT_SIZE 0X200
static char inputBuffer[INPUT_SIZE];
static int inputLength;
static const char *inputDelimiters = " ";

#define OUTPUT_SIZE 0X200
static char outputBuffer[OUTPUT_SIZE];
static int outputLength;

typedef struct {
  const CommandEntry *entry;
  unsigned int maximum;
} CommandDescriptor;
static CommandDescriptor *commandDescriptors = NULL;
static const size_t commandSize = sizeof(*commandDescriptors);
static size_t commandCount;

static int brailleColumns;
static int brailleRows;
static int brailleCells;
static unsigned char *previousBraille = NULL;
static unsigned char *previousVisual = NULL;

static int statusCells = 0;
static unsigned char previousStatus[StatusCellCount];

static char *
formatSocketAddress (const struct sockaddr *address) {
  switch (address->sa_family) {
    case AF_UNIX: {
      const struct sockaddr_un *sa = (const struct sockaddr_un *)address;
      return strdupWrapper(sa->sun_path);
    }

    case AF_INET: {
      const struct sockaddr_in *sa = (const struct sockaddr_in *)address;
      const char *host = inet_ntoa(sa->sin_addr);
      unsigned short port = ntohs(sa->sin_port);
      char buffer[strlen(host) + 7];
      snprintf(buffer, sizeof(buffer), "%s:%u", host, port);
      return strdupWrapper(buffer);
    }

    default:
      return strdupWrapper("");
  }
}

static int
acceptConnection (
  int queueSocket,
  const struct sockaddr *localAddress, socklen_t localSize,
  struct sockaddr *remoteAddress, socklen_t *remoteSize
) {
  int serverSocket = -1;

  if (listen(queueSocket, 1) != -1) {
    int attempts = 0;

    {
      char *address = formatSocketAddress(localAddress);
      LogPrint(LOG_NOTICE, "Listening on: %s", address);
      free(address);
    }

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
          if (!FD_ISSET(queueSocket, &readMask)) continue;

          if ((serverSocket = accept(queueSocket, remoteAddress, remoteSize)) != -1) {
            char *address = formatSocketAddress(remoteAddress);
            LogPrint(LOG_NOTICE, "Client is: %s", address);
            free(address);
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

  return serverSocket;
}

static int
acceptUnixConnection (const char *path) {
  int serverSocket = -1;
  int queueSocket;

  if ((queueSocket = socket(PF_UNIX, SOCK_STREAM, 0)) != -1) {
    struct sockaddr_un localAddress;
    struct sockaddr_un remoteAddress;
    socklen_t remoteSize = sizeof(remoteAddress);

    memset(&localAddress, 0, sizeof(localAddress));
    localAddress.sun_family = AF_UNIX;
    strncpy(localAddress.sun_path, path, sizeof(localAddress.sun_path)-1);

    if (bind(queueSocket, (struct sockaddr *)&localAddress, sizeof(localAddress)) != -1) {
      serverSocket = acceptConnection(queueSocket,
                                      (struct sockaddr *)&localAddress, sizeof(localAddress),
                                      (struct sockaddr *)&remoteAddress, &remoteSize);
      if (unlink(localAddress.sun_path) == -1) {
        LogError("unlink");
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

static int
acceptInetConnection (unsigned short port, unsigned long address) {
  int serverSocket = -1;
  int queueSocket;

  if ((queueSocket = socket(PF_INET, SOCK_STREAM, 0)) != -1) {
    struct sockaddr_in localAddress;
    struct sockaddr_in remoteAddress;
    socklen_t remoteSize = sizeof(remoteAddress);

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
      serverSocket = acceptConnection(queueSocket,
                                      (struct sockaddr *)&localAddress, sizeof(localAddress),
                                      (struct sockaddr *)&remoteAddress, &remoteSize);
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
readCommandLine (void) {
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

static int
flushOutput (void) {
  const char *buffer = outputBuffer;
  size_t length = outputLength;

  while (length) {
    int written = write(fileDescriptor, buffer, length);

    if (written == -1) {
      if (errno == EINTR) continue;
      LogError("write");
      memmove(outputBuffer, buffer, (outputLength = length));
      return 0;
    }

    buffer += written;
    length -= written;
  }

  outputLength = 0;
  return 1;
}

static int
writeBytes (const char *bytes, size_t length) {
  while (length) {
    size_t count = OUTPUT_SIZE - outputLength;
    if (length < count) count = length;
    memcpy(&outputBuffer[outputLength], bytes, count);
    bytes += count;
    length -= count;
    if ((outputLength += count) == OUTPUT_SIZE)
      if (!flushOutput())
        return 0;
  }

  return 1;
}

static int
writeByte (char byte) {
  return writeBytes(&byte, 1);
}

static int
writeString (const char *string) {
  return writeBytes(string, strlen(string));
}

/* Print the dots in buf in a humanly readable form into a string.
 * Caller is responsible for freeing string
 */
static int
writeDots (const unsigned char *cells, int count) {
  const unsigned char *cell = cells;

  while (count--) {
    char dots[9];
    char *d = dots;

    if (cell != cells) *d++ = '|';
    if (*cell) {
      if (*cell & B1) *d++ = '1';
      if (*cell & B2) *d++ = '2';
      if (*cell & B3) *d++ = '3';
      if (*cell & B4) *d++ = '4';
      if (*cell & B5) *d++ = '5';
      if (*cell & B6) *d++ = '6';
      if (*cell & B7) *d++ = '7';
      if (*cell & B8) *d++ = '8';
    } else {
      *d++ = ' ';
    }
    ++cell;

    if (!writeBytes(dots, d-dots)) return 0;
  }

  return 1;
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
  allocateCommandDescriptors();
  inputLength = 0;
  outputLength = 0;

  {
    const char *socket = parameters[PARM_SOCKET];
    int port;

    if (!socket) socket = "";
    if (!*socket) socket = "brltty-vr.socket";

    if (isInteger(&port, socket)) {
      if ((port > 0) && (port <= 0XFFFF)) {
        fileDescriptor = acceptInetConnection(port, INADDR_ANY);
      } else {
        LogPrint(LOG_WARNING, "Invalid port: %d", port);
      }
    } else {
      char *path = makePath(brl->dataDirectory, socket);
      if (path) {
        fileDescriptor = acceptUnixConnection(path);
        free(path);
      }
    }
  }

  if (fileDescriptor != -1) {
    char *line = NULL;

    while (1) {
      if (line) free(line);
      if ((line = readCommandLine())) {
        const char *word;
        LogPrint(LOG_DEBUG, "Command received: %s", line);

        word = strtok(line, inputDelimiters);
        if (strcasecmp(word, "cells") == 0) {
          int ok = 0;

          if ((word = strtok(NULL, inputDelimiters))) {
            if (isInteger(&brailleColumns, word) && (brailleColumns > 0)) {
              ok = 1;

              if ((word = strtok(NULL, inputDelimiters))) {
                if (!isInteger(&brailleRows, word) || (brailleRows < 1)) {
                  LogPrint(LOG_WARNING, "Invalid row count.");
                  ok = 0;
                }
              } else {
                brailleRows = 1;
              }
            } else {
              LogPrint(LOG_WARNING, "Invalid column count.");
            }
          } else {
            LogPrint(LOG_WARNING, "Missing column count.");
          }

          if (ok) {
            brailleCells = brailleColumns * brailleRows;

            if ((previousBraille = mallocWrapper(brailleCells))) {
              memset(previousBraille, 0, brailleCells);

              if ((previousVisual = mallocWrapper(brailleCells))) {
                memset(previousVisual, ' ', brailleCells);

                brl->x = brailleColumns;
                brl->y = brailleRows;
                brl->helpPage = 0;

                memset(previousStatus, 0, sizeof(previousStatus));
                free(line);
                return 1;

                free(previousVisual);
                previousVisual = NULL;
              }

              free(previousBraille);
              previousBraille = NULL;
            }
          }
        } else if (strcasecmp(word, "quit") == 0) {
          LogPrint(LOG_NOTICE, "Client requested termination.");
          break;
        } else {
          LogPrint(LOG_WARNING, "Unexpected command: %s", word);
        }
      } else {
        delay(1000);
      }
    }
    if (line) free(line);

    close(fileDescriptor);
    fileDescriptor = -1;
  }

  return 0;
}

static void
brl_close (BrailleDisplay *brl) {
  if (previousVisual) {
    free(previousVisual);
    previousVisual = NULL;
  }

  if (previousBraille) {
    free(previousBraille);
    previousBraille = NULL;
  }

  if (fileDescriptor != -1) {
    close(fileDescriptor);
    fileDescriptor = -1;
  }
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, previousBraille, brailleCells) != 0) {
    writeString("Braille=\"");
    writeDots(brl->buffer, brailleCells);
    writeString("\"\n");
    flushOutput();

    memcpy(previousBraille, brl->buffer, brailleCells);
  }
}

static void
brl_writeVisual (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, previousVisual, brailleCells) != 0) {
    writeString("Visual=\"");
    {
      const unsigned char *address = brl->buffer;
      int cells = brailleCells;
      while (cells--) {
        unsigned char character = *address++;
        switch (character) {
          case '"':
          case '\\':
            writeByte('\\');
          default:
            writeByte(character);
            break;
        }
      }
    }
    writeString("\"\n");
    flushOutput();

    memcpy(previousVisual, brl->buffer, brailleCells);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st) {
  int generic = *st == FSC_GENERIC;
  int cells = generic? StatusCellCount: statusCells;

  if (memcmp(st, previousStatus, cells) != 0) {
    if (generic) {
      int all = previousStatus[0] != FSC_GENERIC;
      int i;

      for (i=1; i<StatusCellCount; ++i) {
        unsigned char value = st[i];
        if (all || (value != previousStatus[i])) {
          static const char *const names[] = {
            [0 ... (StatusCellCount-1)]=NULL,
            [STAT_BRLCOL]="BRLCOL",
            [STAT_BRLROW]="BRLROW",
            [STAT_CSRCOL]="CSRCOL",
            [STAT_CSRROW]="CSRROW",
            [STAT_SCRNUM]="SCRNUM",
            [STAT_FREEZE]="FREEZE",
            [STAT_DISPMD]="DISPMD",
            [STAT_SIXDOTS]="SIXDOTS",
            [STAT_SLIDEWIN]="SLIDEWIN",
            [STAT_SKPIDLNS]="SKPIDLNS",
            [STAT_SKPBLNKWINS]="SKPBLNKWINS",
            [STAT_CSRVIS]="CSRVIS",
            [STAT_CSRHIDE]="CSRHIDE",
            [STAT_CSRTRK]="CSRTRK",
            [STAT_CSRSIZE]="CSRSIZE",
            [STAT_CSRBLINK]="CSRBLINK",
            [STAT_ATTRVIS]="ATTRVIS",
            [STAT_ATTRBLINK]="ATTRBLINK",
            [STAT_CAPBLINK]="CAPBLINK",
            [STAT_TUNES]="TUNES",
            [STAT_HELP]="HELP",
            [STAT_INFO]="INFO"
          };
          const char *name = names[i];
          if (name) {
            char buffer[0X40];
            snprintf(buffer, sizeof(buffer), "%s %d\n", name, value);
            writeString(buffer);
            flushOutput();
          }
        }
      }
    } else {
      writeString("Status=\"");
      writeDots(st, cells);
      writeString("\"\n");
      flushOutput();
    }

    memcpy(previousStatus, st, cells);
  }
}

static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext context) {
  int command = EOF;
  char *line = readCommandLine();

  if (line) {
    const char *word;
    LogPrint(LOG_DEBUG, "Command received: %s", line);

    if ((word = strtok(line, inputDelimiters))) {
      if (strcasecmp(word, "quit") == 0) {
        command = CMD_RESTARTBRL;
      } else {
        const CommandDescriptor *descriptor = findCommand(word);
        if (descriptor) {
          int needsNumber = descriptor->maximum > 0;
          int numberSpecified = 0;
          int switchSpecified = 0;
          int block;

          command = descriptor->entry->code;
          block = command & VAL_BLK_MASK;

          while ((word = strtok(NULL, inputDelimiters))) {
            if (block == 0) {
              if (!switchSpecified) {
                if (strcasecmp(word, "on") == 0) {
                  switchSpecified = 1;
                  command |= VAL_SWITCHON;
                  continue;
                }

                if (strcasecmp(word, "off") == 0) {
                  switchSpecified = 1;
                  command |= VAL_SWITCHOFF;
                  continue;
                }
              }
            }

            if (needsNumber && !numberSpecified) {
              int number;
              if (isInteger(&number, word)) {
                if ((number > 0) && (number <= descriptor->maximum)) {
                  numberSpecified = 1;
                  command += number;
                  continue;
                }
              }
            }

            LogPrint(LOG_WARNING, "Unknown option: %s", word);
          }

          if (needsNumber && !numberSpecified) {
            LogPrint(LOG_WARNING, "Number not specified.");
            command = EOF;
          }
        } else {
          LogPrint(LOG_WARNING, "Unknown command: %s", word);
        }
      }
    }

    free(line);
  }

  return command;
}
