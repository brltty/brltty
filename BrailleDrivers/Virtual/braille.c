/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __MINGW32__
#include <ws2tcpip.h>
#include "sys_windows.h"
#else /* __MINGW32__ */
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* __MINGW32__ */

#if !defined(AF_LOCAL) && defined(AF_UNIX)
#define AF_LOCAL AF_UNIX
#endif /* !defined(AF_LOCAL) && defined(AF_UNIX) */
 
#if !defined(PF_LOCAL) && defined(PF_UNIX)
#define PF_LOCAL PF_UNIX
#endif /* !defined(PF_LOCAL) && defined(PF_UNIX) */
 
#ifdef WINDOWS
#undef AF_LOCAL
#endif /* WINDOWS */

#ifdef __MINGW32__
#define close(fd) CloseHandle((HANDLE)(fd))
#define LogSocketError(msg) LogWindowsSocketError(msg)
#else /* __MINGW32__ */
#define LogSocketError(msg) LogError(msg)
#endif /* __MINGW32__ */

#include "misc.h"
#include "cmd.h"

#define BRLSTAT ST_Generic
#define BRL_HAVE_STATUS_CELLS
#define BRL_HAVE_VISUAL_DISPLAY
#include "brl_driver.h"
#include "braille.h"

static int fileDescriptor = -1;

#define INPUT_SIZE 0X200
static char inputBuffer[INPUT_SIZE];
static size_t inputLength;
static size_t inputStart;
static int inputEnd;
static int inputCarriageReturn;
static const char *inputDelimiters = " ";

#define OUTPUT_SIZE 0X200
static char outputBuffer[OUTPUT_SIZE];
static size_t outputLength;

typedef struct {
  const CommandEntry *entry;
  unsigned int count;
} CommandDescriptor;
static CommandDescriptor *commandDescriptors = NULL;
static const size_t commandSize = sizeof(*commandDescriptors);
static size_t commandCount;

static int brailleColumns;
static int brailleRows;
static int brailleCells;
static unsigned char *previousBraille = NULL;
static unsigned char *previousVisual = NULL;
static unsigned char previousStatus[BRL_MAX_STATUS_CELL_COUNT];

typedef struct {
#ifdef AF_LOCAL
  int (*getLocalConnection) (const struct sockaddr_un *address);
#endif /* AF_LOCAL */

#ifdef __MINGW32__
  int (*getNamedPipeConnection) (const char *path);
#endif /* __MINGW32__ */

  int (*getInetConnection) (const struct sockaddr_in *address);
} ModeEntry;
static const ModeEntry *mode;

typedef struct {
  int (*read) (int descriptor, void *buffer, int size);
} OperationsEntry;
static const OperationsEntry *operations;

static int
readSocket (int descriptor, void *buffer, int size) {
  fd_set readMask;
  struct timeval timeout;

  FD_ZERO(&readMask);
  FD_SET(fileDescriptor, &readMask);

  memset(&timeout, 0, sizeof(timeout));

  switch (select(fileDescriptor+1, &readMask, NULL, NULL, &timeout)) {
    case -1:
      LogSocketError("select");
      break;

    case 0:
      errno = EAGAIN;
      break;
  
    default: {
      int count = recv(descriptor, buffer, size, 0);
      if (count != -1) return count;
      LogSocketError("recv");
      break;
    }
  }

  return -1;
}

static const OperationsEntry socketOperationsEntry = {
  readSocket
};

static char *
formatSocketAddress (const struct sockaddr *address) {
  switch (address->sa_family) {
#ifdef AF_LOCAL
    case AF_LOCAL: {
      const struct sockaddr_un *localAddress = (const struct sockaddr_un *)address;
      return strdupWrapper(localAddress->sun_path);
    }
#endif /* AF_LOCAL */

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
acceptSocketConnection (
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
            char *address = formatSocketAddress(localAddress);
            LogPrint(LOG_NOTICE, "listening on: %s", address);
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
                LogSocketError("select");
                break;

              case 0:
                LogPrint(LOG_DEBUG, "no connection yet, still waiting (%d).", attempts);
                continue;

              default: {
                if (!FD_ISSET(queueSocket, &readMask)) continue;

                if ((serverSocket = accept(queueSocket, remoteAddress, remoteSize)) != -1) {
                  char *address = formatSocketAddress(remoteAddress);
                  LogPrint(LOG_NOTICE, "client is: %s", address);
                  free(address);
                } else {
                  LogSocketError("accept");
                }
              }
            }
            break;
          }
        } else {
          LogSocketError("listen");
        }

        if (unbindAddress) unbindAddress(localAddress);
      } else {
        LogSocketError("bind");
      }
    }

    close(queueSocket);
  } else {
    LogSocketError("socket");
  }

  operations = &socketOperationsEntry;
  return serverSocket;
}

static int
requestConnection (
  int (*getSocket) (void),
  const struct sockaddr *remoteAddress, socklen_t remoteSize
) {
  int clientSocket;

  {
    char *address = formatSocketAddress(remoteAddress);
    LogPrint(LOG_DEBUG, "connecting to: %s", address);
    free(address);
  }

  if ((clientSocket = getSocket()) != -1) {
    if (connect(clientSocket, remoteAddress, remoteSize) != -1) {
      {
        char *address = formatSocketAddress(remoteAddress);
        LogPrint(LOG_NOTICE, "connected to: %s", address);
        free(address);
      }

      operations = &socketOperationsEntry;
      return clientSocket;
    } else {
      LogPrint(LOG_WARNING, "connect error: %s", strerror(errno));
    }

    close(clientSocket);
  } else {
    LogSocketError("socket");
  }

  return -1;
}

static int
setSocketReuseAddress (int socket) {
  int yes = 1;

  if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes)) != -1) {
    return 1;
  } else {
    LogSocketError("setsockopt REUSEADDR");
  }
  return 0;
}

#ifdef AF_LOCAL
static int
setLocalAddress (const char *string, struct sockaddr_un *address) {
  int ok = 1;

  memset(address, 0, sizeof(*address));
  address->sun_family = AF_LOCAL;

  if (strlen(string) < sizeof(address->sun_path)) {
    strncpy(address->sun_path, string, sizeof(address->sun_path)-1);
  } else {
    ok = 0;
    LogPrint(LOG_WARNING, "Local socket path too long: %s", string);
  }

  return ok;
}

static int
getLocalSocket (void) {
  return socket(PF_LOCAL, SOCK_STREAM, 0);
}

static void
unbindLocalAddress (const struct sockaddr *address) {
  const struct sockaddr_un *localAddress = (const struct sockaddr_un *)address;
  if (unlink(localAddress->sun_path) == -1) {
    LogError("unlink");
  }
}

static int
acceptLocalConnection (const struct sockaddr_un *localAddress) {
  struct sockaddr_un remoteAddress;
  socklen_t remoteSize = sizeof(remoteAddress);

  return acceptSocketConnection(getLocalSocket, NULL, unbindLocalAddress,
                                (const struct sockaddr *)localAddress, sizeof(*localAddress),
                                (struct sockaddr *)&remoteAddress, &remoteSize);
}

static int
requestLocalConnection (const struct sockaddr_un *remoteAddress) {
  return requestConnection(getLocalSocket,
                           (const struct sockaddr *)remoteAddress, sizeof(*remoteAddress));
}
#endif /* AF_LOCAL */

#ifdef __MINGW32__
static int
readNamedPipe (int descriptor, void *buffer, int size) {
  {
    DWORD available;

    if (!PeekNamedPipe((HANDLE)descriptor, NULL, 0, NULL, &available, NULL)) {
      LogWindowsError("PeekNamedPipe");
      return 0;
    }

    if (!available) {
      errno = EAGAIN;
      return -1;
    }

    if (available < size) size = available;
  }

  {
    DWORD received;
    OVERLAPPED overl = {0, 0, 0, 0, NULL};
    overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!ReadFile((HANDLE)descriptor, buffer, size, &received, &overl)) {
      if (GetLastError() != ERROR_IO_PENDING) {
        LogWindowsError("ReadPipe");
        received = 0;
      } else if (!GetOverlappedResult((HANDLE)descriptor, &overl, &received, TRUE)) {
        LogWindowsError("GetOverlappedResult");
        received = 0;
      }
    }

    CloseHandle(overl.hEvent);
    return received;
  }
}

static const OperationsEntry namedPipeOperationsEntry = {
  readNamedPipe
};

static int
acceptNamedPipeConnection (const char *path) {
  HANDLE h;
  OVERLAPPED overl = {0,0,0,0,NULL};
  DWORD res;
  int attempts = 0;

  if ((h = CreateNamedPipe(path, 
                                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                1, 0, 0, 0, NULL)) == INVALID_HANDLE_VALUE) {
    LogWindowsError("CreateNamedPipe");
    return -1;
  }

  overl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (!ConnectNamedPipe(h, &overl)) {
    switch (GetLastError()) {
      case ERROR_IO_PENDING:
        while ((res = WaitForSingleObject(overl.hEvent, 10000)) != WAIT_OBJECT_0) {
          if (res == WAIT_TIMEOUT) {
            ++attempts;
            LogPrint(LOG_DEBUG, "no connection yet, still waiting (%d).", attempts);
          } else {
            LogWindowsError("ConnectNamedPipe");
            CloseHandle(h);
            h = (HANDLE) -1;
            break;
          }
        }

      case ERROR_PIPE_CONNECTED:
        break;

      default:
        LogWindowsError("ConnectNamedPipe");
        CloseHandle(h);
        h = (HANDLE) -1;
        break;
    }
  }

  CloseHandle(overl.hEvent);
  operations = &namedPipeOperationsEntry;
  return (int)h;
}

static int
requestNamedPipeConnection (const char *path) {
  HANDLE h;

  if ((h = CreateFile(path,
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
    LogWindowsError("Connect to named pipe");
    return -1;
  }

  operations = &namedPipeOperationsEntry;
  return (int)h;
}
#endif /* __MINGW32__ */

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
  if (setSocketReuseAddress(socket)) return 1;
  return 0;
}

static int
acceptInetConnection (const struct sockaddr_in *localAddress) {
  struct sockaddr_in remoteAddress;
  socklen_t remoteSize = sizeof(remoteAddress);

  return acceptSocketConnection(getInetSocket, prepareInetQueue, NULL,
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
    int count = operations->read(fileDescriptor, &inputBuffer[inputLength], INPUT_SIZE-inputLength);
    if (!count) {
      inputEnd = 1;
    } else if (count != -1) {
      inputLength += count;
    } else if (errno != EAGAIN) {
      return 0;
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
        inputCarriageReturn = 0;
        if ((newline != inputBuffer) && (*(newline-1) == '\r')) {
          inputCarriageReturn = 1;
          --stringLength;
        }
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
#ifdef __MINGW32__
    DWORD sent;
    OVERLAPPED overl = {0,0,0,0,CreateEvent(NULL,TRUE,FALSE,NULL)};
    if ((!WriteFile((HANDLE) fileDescriptor, buffer, length, &sent, &overl)
      && GetLastError() != ERROR_IO_PENDING) ||
      !GetOverlappedResult((HANDLE) fileDescriptor, &overl, &sent, TRUE)) {
        LogSocketError("WriteFile");
        CloseHandle(overl.hEvent);
        memmove(outputBuffer, buffer, (outputLength = length));
        return 0;
      }
    CloseHandle(overl.hEvent);
#else /* __MINGW32__ */
    int sent;
    sent = send(fileDescriptor, buffer, length, 0);

    if (sent == -1) {
      if (errno == EINTR) continue;
      LogSocketError("send");
      memmove(outputBuffer, buffer, (outputLength = length));
      return 0;
    }
#endif /* __MINGW32__ */

    buffer += sent;
    length -= sent;
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
      if (*cell & BRL_DOT1) *d++ = '1';
      if (*cell & BRL_DOT2) *d++ = '2';
      if (*cell & BRL_DOT3) *d++ = '3';
      if (*cell & BRL_DOT4) *d++ = '4';
      if (*cell & BRL_DOT5) *d++ = '5';
      if (*cell & BRL_DOT6) *d++ = '6';
      if (*cell & BRL_DOT7) *d++ = '7';
      if (*cell & BRL_DOT8) *d++ = '8';
    } else {
      *d++ = ' ';
    }
    ++cell;

    if (!writeBytes(dots, d-dots)) return 0;
  }

  return 1;
}

static int
writeLine (void) {
  if (inputCarriageReturn)
    if (!writeByte('\r'))
      return 0;
  if (writeByte('\n'))
    if (flushOutput())
      return 1;
  return 0;
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
        descriptor->count = 0;
        ++descriptor;
      }
    }

    sortCommandsByCode();
    {
      CommandDescriptor *descriptor = commandDescriptors + commandCount;
      int previousBlock = -1;

      while (descriptor-- != commandDescriptors) {
        int code = descriptor->entry->code;
        int currentBlock = code & BRL_MSK_BLK;

        if (currentBlock != previousBlock) {
          if (currentBlock) {
            descriptor->count = (BRL_MSK_ARG + 1) - (code & BRL_MSK_ARG);
          }
          previousBlock = currentBlock;
        }
      }
    }

    sortCommandsByName();
  }
}

static void
deallocateCommandDescriptors (void) {
  if (commandDescriptors) {
    free(commandDescriptors);
    commandDescriptors = NULL;
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
      }

      free(braille);
    }
  }

  return 0;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  allocateCommandDescriptors();

  inputLength = 0;
  inputStart = 0;
  inputEnd = 0;
  outputLength = 0;

  if (isQualifiedDevice(&device, "client")) {
    static const ModeEntry clientModeEntry = {
#ifdef AF_LOCAL
      requestLocalConnection,
#endif /* AF_LOCAL */

#ifdef __MINGW32__
      requestNamedPipeConnection,
#endif /* __MINGW32__ */

      requestInetConnection
    };
    mode = &clientModeEntry;
  } else if (isQualifiedDevice(&device, "server")) {
    static const ModeEntry serverModeEntry = {
#ifdef AF_LOCAL
      acceptLocalConnection,
#endif /* AF_LOCAL */

#ifdef __MINGW32__
      acceptNamedPipeConnection,
#endif /* __MINGW32__ */

      acceptInetConnection
    };
    mode = &serverModeEntry;
  } else {
    unsupportedDevice(device);
    goto failed;
  }
  if (!*device) device = VR_DEFAULT_SOCKET;

#ifdef AF_LOCAL
  if (device[0] == '/') {
    struct sockaddr_un address;
    if (setLocalAddress(device, &address)) {
      fileDescriptor = mode->getLocalConnection(&address);
    }
  } else
#endif /* AF_LOCAL */

#ifdef __MINGW32__
  if (device[0] == '\\') {
    fileDescriptor = mode->getNamedPipeConnection(device);
  } else {
    static WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1, 1), &wsadata)) {
      LogWindowsError("socket library start");
      goto failed;
    }
  }
#endif /* __MINGW32__ */

  {
    struct sockaddr_in address;
    if (setInetAddress(device, &address)) {
      fileDescriptor = mode->getInetConnection(&address);
    }
  }

  if (fileDescriptor != -1) {
    char *line = NULL;

    while (1) {
      if (line) free(line);
      if ((line = readCommandLine())) {
        const char *word;
        LogPrint(LOG_DEBUG, "command received: %s", line);

        if ((word = strtok(line, inputDelimiters))) {
          if (testWord(word, "cells")) {
            if (dimensionsChanged(brl)) {
              free(line);
              return 1;
            }
          } else if (testWord(word, "quit")) {
            break;
          } else {
            LogPrint(LOG_WARNING, "unexpected command: %s", word);
          }
        }
      } else {
        approximateDelay(1000);
      }
    }
    if (line) free(line);

    close(fileDescriptor);
    fileDescriptor = -1;
  }

failed:
  deallocateCommandDescriptors();
  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
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

  deallocateCommandDescriptors();
}

static void
brl_writeWindow (BrailleDisplay *brl) {
  if (memcmp(brl->buffer, previousBraille, brailleCells) != 0) {
    writeString("Braille \"");
    writeDots(brl->buffer, brailleCells);
    writeString("\"\n");
    writeLine();

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
    writeLine();

    memcpy(previousVisual, brl->buffer, brailleCells);
  }
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *st) {
  int generic = st[BRL_firstStatusCell] == BRL_STATUS_CELLS_GENERIC;
  int cells = BRL_MAX_STATUS_CELL_COUNT;

  if (memcmp(st, previousStatus, cells) != 0) {
    if (generic) {
      int all = previousStatus[BRL_firstStatusCell] != BRL_STATUS_CELLS_GENERIC;
      int i;

      for (i=1; i<BRL_MAX_STATUS_CELL_COUNT; ++i) {
        unsigned char value = st[i];
        if (all || (value != previousStatus[i])) {
          static const char *const names[] = {
            [BRL_firstStatusCell] = NULL,
            [BRL_GSC_BRLCOL] = "BRLCOL",
            [BRL_GSC_BRLROW] = "BRLROW",
            [BRL_GSC_CSRCOL] = "CSRCOL",
            [BRL_GSC_CSRROW] = "CSRROW",
            [BRL_GSC_SCRNUM] = "SCRNUM",
            [BRL_GSC_FREEZE] = "FREEZE",
            [BRL_GSC_DISPMD] = "DISPMD",
            [BRL_GSC_SIXDOTS] = "SIXDOTS",
            [BRL_GSC_SLIDEWIN] = "SLIDEWIN",
            [BRL_GSC_SKPIDLNS] = "SKPIDLNS",
            [BRL_GSC_SKPBLNKWINS] = "SKPBLNKWINS",
            [BRL_GSC_CSRVIS] = "CSRVIS",
            [BRL_GSC_CSRHIDE] = "CSRHIDE",
            [BRL_GSC_CSRTRK] = "CSRTRK",
            [BRL_GSC_CSRSIZE] = "CSRSIZE",
            [BRL_GSC_CSRBLINK] = "CSRBLINK",
            [BRL_GSC_ATTRVIS] = "ATTRVIS",
            [BRL_GSC_ATTRBLINK] = "ATTRBLINK",
            [BRL_GSC_CAPBLINK] = "CAPBLINK",
            [BRL_GSC_TUNES] = "TUNES",
            [BRL_GSC_HELP] = "HELP",
            [BRL_GSC_INFO] = "INFO",
            [BRL_GSC_AUTOREPEAT] = "AUTOREPEAT",
            [BRL_GSC_AUTOSPEAK] = "AUTOSPEAK"
          };
          const int nameCount = ARRAY_COUNT(names);

          if (i < nameCount) {
            const char *name = names[i];
            if (name) {
              char buffer[0X40];
              snprintf(buffer, sizeof(buffer), "%s %d\n", name, value);
              writeString(buffer);
            }
          }
        }
      }
      writeLine();
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
      writeLine();
    }

    memcpy(previousStatus, st, cells);
  }
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
  int command = EOF;
  char *line = readCommandLine();

  if (line) {
    const char *word;
    LogPrint(LOG_DEBUG, "Command received: %s", line);

    if ((word = strtok(line, inputDelimiters))) {
      if (testWord(word, "cells")) {
        if (dimensionsChanged(brl)) brl->resizeRequired = 1;
      } else if (testWord(word, "quit")) {
        command = BRL_CMD_RESTARTBRL;
      } else {
        const CommandDescriptor *descriptor = findCommand(word);
        if (descriptor) {
          int needsNumber = descriptor->count > 0;
          int numberSpecified = 0;
          int switchSpecified = 0;
          int block;

          command = descriptor->entry->code;
          block = command & BRL_MSK_BLK;

          while ((word = nextWord())) {
            if (block == 0) {
              if (!switchSpecified) {
                if (testWord(word, "on")) {
                  switchSpecified = 1;
                  command |= BRL_FLG_TOGGLE_ON;
                  continue;
                }

                if (testWord(word, "off")) {
                  switchSpecified = 1;
                  command |= BRL_FLG_TOGGLE_OFF;
                  continue;
                }
              }
            }

            if (needsNumber && !numberSpecified) {
              int number;
              if (isInteger(&number, word)) {
                if ((number > 0) && (number <= descriptor->count)) {
                  numberSpecified = 1;
                  command += number;
                  continue;
                } else {
                  LogPrint(LOG_WARNING, "Number out of range.");
                }
              }
            }

            LogPrint(LOG_WARNING, "unknown option: %s", word);
          }

          if (needsNumber && !numberSpecified) {
            LogPrint(LOG_WARNING, "Number not specified.");
            command = EOF;
          }
        } else {
          LogPrint(LOG_WARNING, "unknown command: %s", word);
        }
      }
    }

    free(line);
  }

  return command;
}
