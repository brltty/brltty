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
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "Programs/misc.h"
#include "Programs/brl.h"

#define BRLNAME "Virtual"
#define PREFSTYLE ST_Generic

typedef enum {
  PARM_SOCKET,
  PARM_MODE
} DriverParameter;
#define BRLPARMS "socket", "mode"

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "braille.h"

static int fileDescriptor = -1;

#define INPUT_SIZE 0X200
static char inputBuffer[INPUT_SIZE];
static size_t inputLength;
static size_t inputStart;
static int inputEnd;
static const char *inputDelimiters = " ";

#define OUTPUT_SIZE 0X200
static char outputBuffer[OUTPUT_SIZE];
static size_t outputLength;

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
static unsigned char previousStatus[StatusCellCount];

static char *
formatAddress (const struct sockaddr *address) {
  switch (address->sa_family) {
    case AF_UNIX: {
      const struct sockaddr_un *unixAddress = (const struct sockaddr_un *)address;
      return strdupWrapper(unixAddress->sun_path);
    }

    case AF_INET: {
      const struct sockaddr_in *inetAddress = (const struct sockaddr_in *)address;
      const char *host = inet_ntoa(inetAddress->sin_addr);
      unsigned short port = ntohs(inetAddress->sin_port);
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
  int (*getSocket) (void),
  int (*prepareQueue) (int socket),
  void (*unbindAddress) (const struct sockaddr *address),
  const struct sockaddr *localAddress, socklen_t localSize,
  struct sockaddr *remoteAddress, socklen_t *remoteSize
) {
  int serverSocket = -1;
  int queueSocket;

  if ((queueSocket = getSocket()) != -1) {
    if (!prepareQueue || prepareQueue(queueSocket)) {
      if (bind(queueSocket, localAddress, localSize) != -1) {
        if (listen(queueSocket, 1) != -1) {
          int attempts = 0;

          {
            char *address = formatAddress(localAddress);
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
                  char *address = formatAddress(remoteAddress);
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

        if (unbindAddress) unbindAddress(localAddress);
      } else {
        LogError("bind");
      }
    }

    close(queueSocket);
  } else {
    LogError("socket");
  }

  return serverSocket;
}

static int
requestConnection (
  int (*getSocket) (void),
  const struct sockaddr *remoteAddress, socklen_t remoteSize
) {
  int clientSocket;

  {
    char *address = formatAddress(remoteAddress);
    LogPrint(LOG_DEBUG, "Connecting to: %s", address);
    free(address);
  }

  if ((clientSocket = getSocket()) != -1) {
    if (connect(clientSocket, remoteAddress, remoteSize) != -1) {
      {
        char *address = formatAddress(remoteAddress);
        LogPrint(LOG_NOTICE, "Connected to: %s", address);
        free(address);
      }

      return clientSocket;
    } else {
      LogPrint(LOG_WARNING, "connect error: %s", strerror(errno));
    }

    close(clientSocket);
  } else {
    LogError("socket");
  }

  return -1;
}

static int
setReuseAddress (int socket) {
  int yes = 1;
  if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != -1) {
    return 1;
  } else {
    LogError("setsockopt REUSEADDR");
  }
  return 0;
}

static int
setUnixAddress (const char *string, struct sockaddr_un *address) {
  int ok = 1;

  memset(address, 0, sizeof(*address));
  address->sun_family = AF_UNIX;

  if (strlen(string) < sizeof(address->sun_path)) {
    strncpy(address->sun_path, string, sizeof(address->sun_path)-1);
  } else {
    ok = 0;
    LogPrint(LOG_WARNING, "Unix socket path too long: %s", string);
  }

  return ok;
}

static int
getUnixSocket (void) {
  return socket(PF_UNIX, SOCK_STREAM, 0);
}

static void
unbindUnixAddress (const struct sockaddr *address) {
  const struct sockaddr_un *unixAddress = (const struct sockaddr_un *)address;
  if (unlink(unixAddress->sun_path) == -1) {
    LogError("unlink");
  }
}

static int
acceptUnixConnection (const struct sockaddr_un *localAddress) {
  struct sockaddr_un remoteAddress;
  socklen_t remoteSize = sizeof(remoteAddress);

  return acceptConnection(getUnixSocket, NULL, unbindUnixAddress,
                          (const struct sockaddr *)localAddress, sizeof(*localAddress),
                          (struct sockaddr *)&remoteAddress, &remoteSize);
}

static int
requestUnixConnection (const struct sockaddr_un *remoteAddress) {
  return requestConnection(getUnixSocket,
                           (const struct sockaddr *)remoteAddress, sizeof(*remoteAddress));
}

static int
setInetAddress (const char *string, struct sockaddr_in *address) {
  int ok = 1;
  char *hostName = strdupWrapper(string);
  char *portNumber = strchr(hostName, ':');

  if (portNumber) {
    *portNumber++ = 0;
  } else {
    portNumber = "";
  }

  memset(address, 0, sizeof(*address));
  address->sin_family = AF_INET;

  if (*hostName) {
    const struct hostent *host = gethostbyname(hostName);
    if (host && (host->h_addrtype == AF_INET) && (host->h_length == sizeof(address->sin_addr))) {
      memcpy(&address->sin_addr, host->h_addr, sizeof(address->sin_addr));
    } else {
      ok = 0;
      LogPrint(LOG_WARNING, "Unknown host name: %s", hostName);
    }
    endhostent();
  } else {
    address->sin_addr.s_addr = INADDR_ANY;
  }

  if (*portNumber) {
    int port;
    if (isInteger(&port, portNumber)) {
      if ((port > 0) && (port <= 0XFFFF)) {
        address->sin_port = htons(port);
      } else {
        ok = 0;
        LogPrint(LOG_WARNING, "Invalid port number: %s", portNumber);
      }
    } else {
      const struct servent *service = getservbyname(portNumber, "tcp");
      if (service) {
        address->sin_port = service->s_port;
      } else {
        ok = 0;
        LogPrint(LOG_WARNING, "Unknown service: %s", portNumber);
      }
      endservent();
    }
  } else {
    address->sin_port = htons(VR_DEFAULT_PORT);
  }

  free(hostName);
  return ok;
}

static int
getInetSocket (void) {
  return socket(PF_INET, SOCK_STREAM, 0);
}

static int
prepareInetQueue (int socket) {
  if (setReuseAddress(socket))
    return 1;
  return 0;
}

static int
acceptInetConnection (const struct sockaddr_in *localAddress) {
  struct sockaddr_in remoteAddress;
  socklen_t remoteSize = sizeof(remoteAddress);

  return acceptConnection(getInetSocket, prepareInetQueue, NULL,
                          (const struct sockaddr *)localAddress, sizeof(*localAddress),
                          (struct sockaddr *)&remoteAddress, &remoteSize);
}

static int
requestInetConnection (const struct sockaddr_in *remoteAddress) {
  return requestConnection(getInetSocket,
                           (const struct sockaddr *)remoteAddress, sizeof(*remoteAddress));
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

static int
fillInputBuffer (void) {
  if ((inputLength < INPUT_SIZE) && !inputEnd) {
    fd_set readMask;
    struct timeval timeout;

    FD_ZERO(&readMask);
    FD_SET(fileDescriptor, &readMask);

    memset(&timeout, 0, sizeof(timeout));

    switch (select(fileDescriptor+1, &readMask, NULL, NULL, &timeout)) {
      case -1:
        LogError("select");
        return 0;

      case 0:
        break;
    
      default: {
        int received = read(fileDescriptor, &inputBuffer[inputLength], INPUT_SIZE-inputLength);

        if (received == -1) {
          LogError("read");
          return 0;
        }

        if (received) 
          inputLength += received;
        else
          inputEnd = 1;
        break;
      }
    }
  }

  return 1;
}

static char *
readCommandLine (void) {
  if (fillInputBuffer()) {
    if (inputStart < inputLength) {
      const char *newline = memchr(&inputBuffer[inputStart], '\n', inputLength-inputStart);
      if (newline) {
        char *string;
        int stringLength = newline - inputBuffer;
        if ((newline != inputBuffer) && (*(newline-1) == '\r')) --stringLength;
        string = makeString(inputBuffer, stringLength);
        inputLength -= ++newline - inputBuffer;
        memmove(inputBuffer, newline, inputLength);
        inputStart = 0;
        return string;
      } else {
        inputStart = inputLength;
      }
    } else if (inputEnd) {
      char *string;
      if (inputLength) {
        string = makeString(inputBuffer, inputLength);
        inputLength = 0;
        inputStart = 0;
      } else {
        string = copyString("quit");
      }
      return string;
    }
  }
  return NULL;
}

static const char *
nextWord (void) {
  return strtok(NULL, inputDelimiters);
}

static int
compareWords (const char *word1, const char *word2) {
  return strcasecmp(word1, word2);
}

static int
testWord (const char *suppliedWord, const char *desiredWord) {
  return compareWords(suppliedWord, desiredWord) == 0;
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

static int
writeDots (const unsigned char *cells, int count) {
  const unsigned char *cell = cells;

  while (count-- > 0) {
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
sortCommands (int (*compareCommands) (const void *item1, const void *item2)) {
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
sortCommandsByCode (void) {
  sortCommands(compareCommandCodes);
}

static int
compareCommandNames (const void *item1, const void *item2) {
  const CommandDescriptor *descriptor1 = item1;
  const CommandDescriptor *descriptor2 = item2;

  return strcmp(descriptor1->entry->name, descriptor2->entry->name);
}

static void
sortCommandsByName (void) {
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

    sortCommandsByCode();
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

    sortCommandsByName();
  }
}

static int
compareCommandName (const void *key, const void *item) {
  const char *name = key;
  const CommandDescriptor *descriptor = item;
  return compareWords(name, descriptor->entry->name);
}

static const CommandDescriptor *
findCommand (const char *name) {
  return bsearch(name, commandDescriptors, commandCount, commandSize, compareCommandName);
}

static int
dimensionsChanged (BrailleDisplay *brl) {
  int ok = 0;
  int columns;
  int rows;
  const char *word;

  if ((word = nextWord())) {
    if (isInteger(&columns, word) && (columns > 0)) {
      ok = 1;

      if ((word = nextWord())) {
        if (!isInteger(&rows, word) || (rows < 1)) {
          LogPrint(LOG_WARNING, "Invalid row count.");
          ok = 0;
        }
      } else {
        rows = 1;
      }
    } else {
      LogPrint(LOG_WARNING, "Invalid column count.");
    }
  } else {
    LogPrint(LOG_WARNING, "Missing column count.");
  }

  if (ok) {
    int cells = columns * rows;
    unsigned char *braille;
    unsigned char *visual;

    if ((braille = malloc(cells))) {
      if ((visual = malloc(cells))) {
        brailleColumns = columns;
        brailleRows = rows;
        brailleCells = cells;

        memset(braille, 0, cells);
        if (previousBraille) free(previousBraille);
        previousBraille = braille;

        memset(visual, ' ', cells);
        if (previousVisual) free(previousVisual);
        previousVisual = visual;

        memset(previousStatus, 0, sizeof(previousStatus));

        brl->x = columns;
        brl->y = rows;
        brl->helpPage = 0;
        return 1;

        free(visual);
      }

      free(braille);
    }
  }

  return 0;
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
  inputStart = 0;
  inputEnd = 0;
  outputLength = 0;

  {
    typedef struct {
      int (*getUnixConnection) (const struct sockaddr_un *address);
      int (*getInetConnection) (const struct sockaddr_in *address);
    } ModeEntry;
    const ModeEntry *mode;

    {
      static const ModeEntry modeTable[] = {
        {requestUnixConnection, requestInetConnection},
        {acceptUnixConnection , acceptInetConnection }
      };
      const char *modes[] = {"client", "server", NULL};
      int modeIndex;
      mode = validateChoice(&modeIndex, "mode", parameters[PARM_MODE], modes)?
               &modeTable[modeIndex]:
               NULL;
    }

    if (mode) {
      const char *socket = parameters[PARM_SOCKET];
      if (!*socket) socket = VR_DEFAULT_SOCKET;
      if (socket[0] == '/') {
        struct sockaddr_un address;
        if (setUnixAddress(socket, &address)) {
          fileDescriptor = mode->getUnixConnection(&address);
        }
      } else {
        struct sockaddr_in address;
        if (setInetAddress(socket, &address)) {
          fileDescriptor = mode->getInetConnection(&address);
        }
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

        if ((word = strtok(line, inputDelimiters))) {
          if (testWord(word, "cells")) {
            if (dimensionsChanged(brl)) {
              free(line);
              return 1;
            }
          } else if (testWord(word, "quit")) {
            break;
          } else {
            LogPrint(LOG_WARNING, "Unexpected command: %s", word);
          }
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
    writeString("Braille \"");
    writeDots(brl->buffer, brailleCells);
    writeString("\"\n");
    flushOutput();

    memcpy(previousBraille, brl->buffer, brailleCells);
  }
}

static void
brl_writeVisual (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, previousVisual, brailleCells) != 0) {
    writeString("Visual \"");
    {
      const unsigned char *address = brl->buffer;
      int cells = brailleCells;

      while (cells-- > 0) {
        unsigned char character = *address++;
        if (iscntrl(character)) {
          char buffer[5];
          snprintf(buffer, sizeof(buffer), "\\X%02X", character);
          writeString(buffer);
        } else {
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
    }
    writeString("\"\n");
    flushOutput();

    memcpy(previousVisual, brl->buffer, brailleCells);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st) {
  int generic = *st == FSC_GENERIC;
  int cells = StatusCellCount;

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
          }
        }
      }
      flushOutput();
    } else {
      while (cells) {
        if (st[--cells]) {
          ++cells;
          break;
        }
      }

      writeString("Status \"");
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
      if (testWord(word, "cells")) {
        if (dimensionsChanged(brl)) brl->resizeRequired = 1;
      } else if (testWord(word, "quit")) {
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

          while ((word = nextWord())) {
            if (block == 0) {
              if (!switchSpecified) {
                if (testWord(word, "on")) {
                  switchSpecified = 1;
                  command |= VAL_SWITCHON;
                  continue;
                }

                if (testWord(word, "off")) {
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
                } else {
                  LogPrint(LOG_WARNING, "Number out of range.");
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
