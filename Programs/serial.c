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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef __MINGW32__
#include <windows.h>
#include <io.h>
typedef DWORD SerialSpeed;
typedef DCB SerialAttributes;
#else /* __MINGW32__ */
#include <sys/ioctl.h>
#include <termios.h>
typedef speed_t SerialSpeed;
typedef struct termios SerialAttributes;
#endif /* __MINGW32__ */

#ifdef HAVE_SYS_MODEM_H
#include <sys/modem.h>
#endif /* HAVE_SYS_MODEM_H */

#include "serial.h"
#include "iomisc.h"
#include "misc.h"

struct SerialDeviceStruct {
  int fileDescriptor;
  SerialAttributes originalAttributes;
  SerialAttributes currentAttributes;
  SerialAttributes pendingAttributes;
  FILE *stream;

#ifdef __MINGW32__
  HANDLE fileHandle;
#endif /* __MINGW32__ */
};

typedef struct {
  int baud;
  SerialSpeed speed;
} BaudEntry;
static const BaudEntry baudTable[] = {
#ifdef __MINGW32__

#ifdef CBR_110
  {110, CBR_110},
#endif /* CBR_110 */

#ifdef CBR_300
  {300, CBR_300},
#endif /* CBR_300 */

#ifdef CBR_600
  {600, CBR_600},
#endif /* CBR_600 */

#ifdef CBR_1200
  {1200, CBR_1200},
#endif /* CBR_1200 */

#ifdef CBR_2400
  {2400, CBR_2400},
#endif /* CBR_2400 */

#ifdef CBR_4800
  {4800, CBR_4800},
#endif /* CBR_4800 */

#ifdef CBR_9600
  {9600, CBR_9600},
#endif /* CBR_9600 */

#ifdef CBR_14400
  {14400, CBR_14400},
#endif /* CBR_14400 */

#ifdef CBR_19200
  {19200, CBR_19200},
#endif /* CBR_19200 */

#ifdef CBR_38400
  {38400, CBR_38400},
#endif /* CBR_38400 */

#ifdef CBR_56000
  {56000, CBR_56000},
#endif /* CBR_56000 */

#ifdef CBR_57600
  {57600, CBR_57600},
#endif /* CBR_57600 */

#ifdef CBR_115200
  {115200, CBR_115200},
#endif /* CBR_115200 */

#ifdef CBR_256000
  {256000, CBR_256000},
#endif /* CBR_256000 */

  {0, 0}

#else /* __MINGW32__ */

#ifdef B50
  {50, B50},
#endif /* B50 */

#ifdef B75
  {75, B75},
#endif /* B75 */

#ifdef B110
  {110, B110},
#endif /* B110 */

#ifdef B134
  {134, B134},
#endif /* B134 */

#ifdef B150
  {150, B150},
#endif /* B150 */

#ifdef B200
  {200, B200},
#endif /* B200 */

#ifdef B300
  {300, B300},
#endif /* B300 */

#ifdef B600
  {600, B600},
#endif /* B600 */

#ifdef B1200
  {1200, B1200},
#endif /* B1200 */

#ifdef B1800
  {1800, B1800},
#endif /* B1800 */

#ifdef B2400
  {2400, B2400},
#endif /* B2400 */

#ifdef B4800
  {4800, B4800},
#endif /* B4800 */

#ifdef B9600
  {9600, B9600},
#endif /* B9600 */

#ifdef B19200
  {19200, B19200},
#endif /* B19200 */

#ifdef B38400
  {38400, B38400},
#endif /* B38400 */

#ifdef B57600
  {57600, B57600},
#endif /* B57600 */

#ifdef B115200
  {115200, B115200},
#endif /* B115200 */

#ifdef B230400
  {230400, B230400},
#endif /* B230400 */

#ifdef B460800
  {460800, B460800},
#endif /* B460800 */

#ifdef B500000
  {500000, B500000},
#endif /* B500000 */

#ifdef B576000
  {576000, B576000},
#endif /* B576000 */

#ifdef B921600
  {921600, B921600},
#endif /* B921600 */

#ifdef B1000000
  {1000000, B1000000},
#endif /* B1000000 */

#ifdef B1152000
  {1152000, B1152000},
#endif /* B1152000 */

#ifdef B1500000
  {1500000, B1500000},
#endif /* B1500000 */

#ifdef B2000000
  {2000000, B2000000},
#endif /* B2000000 */

#ifdef B2500000
  {2500000, B2500000},
#endif /* B2500000 */

#ifdef B3000000
  {3000000, B3000000},
#endif /* B3000000 */

#ifdef B3500000
  {3500000, B3500000},
#endif /* B3500000 */

#ifdef B4000000
  {4000000, B4000000},
#endif /* B4000000 */

  {0, B0}

#endif /* __MINGW32__ */
};

static const BaudEntry *
getBaudEntry (int baud) {
  const BaudEntry *entry = baudTable;
  while (entry->baud) {
    if (baud == entry->baud) return entry;
    ++entry;
  }
  return NULL;
}

static void
serialInitializeAttributes (SerialAttributes *attributes) {
  memset(attributes, 0, sizeof(*attributes));
#ifdef __MINGW32__
  attributes->DCBlength = sizeof(*attributes);
  attributes->fBinary = TRUE;
  attributes->ByteSize = 8;
  attributes->BaudRate = CBR_9600;
  attributes->fRtsControl = RTS_CONTROL_ENABLE;
  attributes->fDtrControl = DTR_CONTROL_ENABLE;
  attributes->fTXContinueOnXoff = TRUE;
  attributes->XonChar = 0X11;
  attributes->XoffChar = 0X13;
#else /* __MINGW32__ */
  attributes->c_cflag = CREAD | CLOCAL | CS8;
  attributes->c_iflag = IGNPAR | IGNBRK;

#ifdef IEXTEN
  attributes->c_lflag = IEXTEN;
#endif /* IEXTEN */

#ifdef _POSIX_VDISABLE
  if (_POSIX_VDISABLE) {
    int i;
    for (i=0; i<NCCS; ++i) {
      if (i == VTIME) continue;
      if (i == VMIN) continue;
      attributes->c_cc[i] = _POSIX_VDISABLE;
    }
  }
#endif /* _POSIX_VDISABLE */
#endif /* __MINGW32__ */
}

static int
serialSetSpeed (SerialDevice *serial, SerialSpeed speed) {
#ifdef __MINGW32__
  serial->pendingAttributes.BaudRate = speed;
  return 1;
#else /* __MINGW32__ */
  if (cfsetispeed(&serial->pendingAttributes, speed) != -1) {
    if (cfsetospeed(&serial->pendingAttributes, speed) != -1) {
      return 1;
    } else {
      LogError("cfsetospeed");
    }
  } else {
    LogError("cfsetispeed");
  }
  return 0;
#endif /* __MINGW32__ */
}

int
serialSetBaud (SerialDevice *serial, int baud) {
  const BaudEntry *entry = getBaudEntry(baud);
  if (entry) {
    if (serialSetSpeed(serial, entry->speed)) {
      return 1;
    }
  } else {
    LogPrint(LOG_WARNING, "Unknown serial baud: %d", baud);
  }
  return 0;
}

int
serialSetDataBits (SerialDevice *serial, int bits) {
#ifdef __MINGW32__
  if ((bits < 5) || (bits > 8)) {
#else /* __MINGW32__ */
  tcflag_t size;
  switch (bits) {
#ifdef CS5
    case 5: size = CS5; break;
#endif /* CS5 */

#ifdef CS6
    case 6: size = CS6; break;
#endif /* CS6 */

#ifdef CS7
    case 7: size = CS7; break;
#endif /* CS7 */

#ifdef CS8
    case 8: size = CS8; break;
#endif /* CS8 */

    default:
#endif /* __MINGW32__ */
      LogPrint(LOG_WARNING, "Unknown serial data bit count: %d", bits);
      return 0;
  }

#ifdef __MINGW32__
  serial->pendingAttributes.ByteSize = bits;
#else /* __MINGW32__ */
  serial->pendingAttributes.c_cflag &= ~CSIZE;
  serial->pendingAttributes.c_cflag |= size;
#endif /* __MINGW32__ */
  return 1;
}

int
serialSetStopBits (SerialDevice *serial, int bits) {
#ifdef __MINGW32__
  if (bits == 1) {
    serial->pendingAttributes.StopBits = ONESTOPBIT;
  } else if (bits == 15) {
    serial->pendingAttributes.StopBits = ONE5STOPBITS;
  } else if (bits == 2) {
    serial->pendingAttributes.StopBits = TWOSTOPBITS;
#else /* __MINGW32__ */
  if (bits == 1) {
    serial->pendingAttributes.c_cflag &= ~CSTOPB;
  } else if (bits == 2) {
    serial->pendingAttributes.c_cflag |= CSTOPB;
#endif /* __MINGW32__ */
  } else {
    LogPrint(LOG_WARNING, "Unknown serial stop bit count: %d", bits);
    return 0;
  }
  return 1;
}

int
serialSetParity (SerialDevice *serial, SerialParity parity) {
#ifdef __MINGW32__
  serial->pendingAttributes.fParity = FALSE;
  serial->pendingAttributes.Parity = NOPARITY;

  if (parity != SERIAL_PARITY_NONE) {
    switch (parity) {
      case SERIAL_PARITY_ODD:
        serial->pendingAttributes.Parity = ODDPARITY;
        break;

      case SERIAL_PARITY_EVEN:
        serial->pendingAttributes.Parity = EVENPARITY;
        break;

      case SERIAL_PARITY_MARK:
        serial->pendingAttributes.Parity = MARKPARITY;
        break;

      case SERIAL_PARITY_SPACE:
        serial->pendingAttributes.Parity = SPACEPARITY;
        break;

      default:
        LogPrint(LOG_WARNING, "Unknown serial parity: %d", parity);
        return 0;
    }

    serial->pendingAttributes.fParity = TRUE;
  }
#else /* __MINGW32__ */
  serial->pendingAttributes.c_cflag &= ~(PARENB | PARODD);

#ifdef PARSTK
  serial->pendingAttributes.c_cflag &= ~PARSTK;
#endif /* PARSTK */

  if (parity != SERIAL_PARITY_NONE) {
    if (parity == SERIAL_PARITY_EVEN) {

    } else if (parity == SERIAL_PARITY_ODD) {
      serial->pendingAttributes.c_cflag |= PARODD;

#ifdef PARSTK
    } else if (parity == SERIAL_PARITY_SPACE) {
      serial->pendingAttributes.c_cflag |= PARSTK;

    } else if (parity == SERIAL_PARITY_MARK) {
      serial->pendingAttributes.c_cflag |= PARSTK | PARODD;
#endif /* PARSTK */

    } else {
      LogPrint(LOG_WARNING, "Unknown serial parity: %d", parity);
      return 0;
    }

    serial->pendingAttributes.c_cflag |= PARENB;
  }
#endif /* __MINGW32__ */
  return 1;
}

int
serialSetFlowControl (SerialDevice *serial, SerialFlowControl flow) {
#ifdef __MINGW32__
  if (flow & SERIAL_FLOW_INPUT_RTS) {
    flow &= ~SERIAL_FLOW_INPUT_RTS;
    serial->pendingAttributes.fRtsControl = RTS_CONTROL_HANDSHAKE;
  } else {
    serial->pendingAttributes.fRtsControl = RTS_CONTROL_ENABLE;
  }

  if (flow & SERIAL_FLOW_INPUT_DTR) {
    flow &= ~SERIAL_FLOW_INPUT_DTR;
    serial->pendingAttributes.fDtrControl = DTR_CONTROL_HANDSHAKE;
  } else {
    serial->pendingAttributes.fDtrControl = DTR_CONTROL_ENABLE;
  }

  if (flow & SERIAL_FLOW_INPUT_XON) {
    flow &= ~SERIAL_FLOW_INPUT_XON;
    serial->pendingAttributes.fInX = TRUE;
  } else {
    serial->pendingAttributes.fInX = FALSE;
  }

  if (flow & SERIAL_FLOW_OUTPUT_CTS) {
    flow &= ~SERIAL_FLOW_OUTPUT_CTS;
    serial->pendingAttributes.fOutxCtsFlow = TRUE;
  } else {
    serial->pendingAttributes.fOutxCtsFlow = FALSE;
  }

  if (flow & SERIAL_FLOW_OUTPUT_DSR) {
    flow &= ~SERIAL_FLOW_OUTPUT_DSR;
    serial->pendingAttributes.fOutxDsrFlow = TRUE;
  } else {
    serial->pendingAttributes.fOutxDsrFlow = FALSE;
  }

  if (flow & SERIAL_FLOW_OUTPUT_XON) {
    flow &= ~SERIAL_FLOW_OUTPUT_XON;
    serial->pendingAttributes.fOutX = TRUE;
  } else {
    serial->pendingAttributes.fOutX = FALSE;
  }
#else /* __MINGW32__ */
  typedef struct {
    tcflag_t *field;
    tcflag_t flag;
    SerialFlowControl flow;
  } FlowControlEntry;
  const FlowControlEntry flowControlTable[] = {
#ifdef CRTSCTS
    {&serial->pendingAttributes.c_cflag, CRTSCTS, SERIAL_FLOW_INPUT_RTS | SERIAL_FLOW_OUTPUT_CTS},
#endif /* CRTSCTS */

#ifdef IHFLOW
    {&serial->pendingAttributes.c_cflag, IHFLOW, SERIAL_FLOW_INPUT_RTS},
#endif /* IHFLOW */

#ifdef OHFLOW
    {&serial->pendingAttributes.c_cflag, OHFLOW, SERIAL_FLOW_OUTPUT_CTS},
#endif /* OHFLOW */

#ifdef IXOFF
    {&serial->pendingAttributes.c_iflag, IXOFF, SERIAL_FLOW_INPUT_XON},
#endif /* IXOFF */

#ifdef IXON
    {&serial->pendingAttributes.c_iflag, IXON, SERIAL_FLOW_OUTPUT_XON},
#endif /* IXON */

#ifdef linux
    {&serial->pendingAttributes.c_cflag, 0x20000000, SERIAL_FLOW_INPUT_CTS},
#endif /* linux */

    {NULL, 0, 0}
  };
  const FlowControlEntry *entry = flowControlTable;

  while (entry->field) {
    if ((flow & entry->flow) == entry->flow) {
      flow &= ~entry->flow;
      *entry->field |= entry->flag;
    } else if (!(flow & entry->flow)) {
      *entry->field &= ~entry->flag;
    }
    ++entry;
  }
#endif /* __MINGW32__ */

  if (!flow) return 1;
  LogPrint(LOG_WARNING, "Unknown serial flow control: 0X%02X", flow);
  return 0;
}

int
serialDiscardInput (SerialDevice *serial) {
#ifdef __MINGW32__
  if (PurgeComm(serial->fileHandle, PURGE_RXABORT|PURGE_RXCLEAR)) return 1;
  LogWindowsError("PurgeComm");
#else /* __MINGW32__ */
  if (tcflush(serial->fileDescriptor, TCIFLUSH) != -1) return 1;
  LogError("TCIFLUSH");
#endif /* __MINGW32__ */
  return 0;
}

int
serialDiscardOutput (SerialDevice *serial) {
#ifdef __MINGW32__
  if (PurgeComm(serial->fileHandle, PURGE_TXABORT|PURGE_TXCLEAR)) return 1;
  LogWindowsError("PurgeComm");
#else /* __MINGW32__ */
  if (tcflush(serial->fileDescriptor, TCOFLUSH) != -1) return 1;
  LogError("TCOFLUSH");
#endif /* __MINGW32__ */
  return 0;
}

int
serialFlushOutput (SerialDevice *serial) {
  if (serial->stream) {
    if (fflush(serial->stream) == EOF) {
      LogError("fflush");
      return 0;
    }
  }
  return 1;
}

int
serialDrainOutput (SerialDevice *serial) {
  if (!serialFlushOutput(serial)) return 0;

#ifdef __MINGW32__
  if (FlushFileBuffers(serial->fileHandle)) return 1;
  LogWindowsError("FlushFileBuffers");
#else /* __MINGW32__ */
  do {
    if (tcdrain(serial->fileDescriptor) != -1) return 1;
  } while (errno == EINTR);
  LogError("tcdrain");
#endif /* __MINGW32__ */
  return 0;
}

static void
serialCopyAttributes (SerialAttributes *destination, const SerialAttributes *source) {
  memcpy(destination, source, sizeof(*destination));
}

static int
serialCompareAttributes (const SerialAttributes *attributes, const SerialAttributes *reference) {
  return memcmp(attributes, reference, sizeof(*attributes)) == 0;
}

static int
serialReadAttributes (SerialDevice *serial) {
#ifdef __MINGW32__
  if (GetCommState(serial->fileHandle, &serial->currentAttributes)) return 1;
  LogWindowsError("GetCommState");
#else /* __MINGW32__ */
  if (tcgetattr(serial->fileDescriptor, &serial->currentAttributes) != -1) return 1;
  LogError("tcgetattr");
#endif /* __MINGW32__ */
  return 0;
}

static int
serialWriteAttributes (SerialDevice *serial, const SerialAttributes *attributes) {
  if (!serialCompareAttributes(attributes, &serial->currentAttributes)) {
    if (!serialDrainOutput(serial)) return 0;

#ifdef __MINGW32__
    if (!SetCommState(serial->fileHandle, (SerialAttributes *)attributes)) {
      LogWindowsError("SetCommState");
      return 0;
    }
#else /* __MINGW32__ */
    if (tcsetattr(serial->fileDescriptor, TCSANOW, attributes) == -1) {
      LogError("tcsetattr");
      return 0;
    }
#endif /* __MINGW32__ */

    serialCopyAttributes(&serial->currentAttributes, attributes);
  }
  return 1;
}

static int
serialFlushAttributes (SerialDevice *serial) {
  return serialWriteAttributes(serial, &serial->pendingAttributes);
}

int
isSerialDevice (const char **path) {
  if (isQualifiedDevice(path, "serial")) return 1;
  return !isQualifiedDevice(path, NULL);
}

int
serialValidateBaud (int *baud, const char *description, const char *word, const int *choices) {
  if (!*word || isInteger(baud, word)) {
    const BaudEntry *entry = getBaudEntry(*baud);
    if (entry) {
      if (!choices) return 1;

      while (*choices) {
        if (*baud == *choices) return 1;
        ++choices;
      }

      LogPrint(LOG_ERR, "Unsupported %s: %d", description, *baud);
      return 0;
    }
  }

  LogPrint(LOG_ERR, "Invalid %s: %d", description, *baud);
  return 0;
}

SerialDevice *
serialOpenDevice (const char *path) {
  SerialDevice *serial;
  if ((serial = malloc(sizeof(*serial)))) {
    char *device;
    if ((device = getDevicePath(path))) {
#ifdef __MINGW32__
      if ((serial->fileHandle = CreateFile(device, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL)) != INVALID_HANDLE_VALUE) {
        serial->fileDescriptor = -1;
#else /* __MINGW32__ */
      if ((serial->fileDescriptor = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK)) != -1) {
        if (isatty(serial->fileDescriptor)) {
#endif /* __MINGW32__ */
          if (serialReadAttributes(serial)) {
            serialCopyAttributes(&serial->originalAttributes, &serial->currentAttributes);
            serialInitializeAttributes(&serial->pendingAttributes);

            serial->stream = NULL;

            LogPrint(LOG_DEBUG, "Serial device opened: %s: fd=%d",
                     device,
#ifdef __MINGW32__
		     (int)serial->fileHandle
#else /* __MINGW32__ */
		     serial->fileDescriptor
#endif /* __MINGW32__ */
		     );
            free(device);
            return serial;
          }
#ifdef __MINGW32__
        CloseHandle(serial->fileHandle);
#else /* __MINGW32__ */
        } else {
          LogPrint(LOG_ERR, "Not a serial device: %s", device);
        }
        close(serial->fileDescriptor);
#endif /* __MINGW32__ */
      } else {
        LogPrint(LOG_ERR, "Cannot open '%s': %s", device, strerror(errno));
      }

      free(device);
    }

    free(serial);
  } else {
    LogError("serial device allocation");
  }
  return NULL;
}

void
serialCloseDevice (SerialDevice *serial) {
  serialWriteAttributes(serial, &serial->originalAttributes);

  if (serial->stream) {
    fclose(serial->stream);
#ifdef __MINGW32__
  } else if (serial->fileDescriptor == -1) {
    CloseHandle(serial->fileHandle);
#endif /* __MINGW32__ */
  } else {
    close(serial->fileDescriptor);
  }

  free(serial);
}

int
serialRestartDevice (SerialDevice *serial, int baud) {
  if (!serialDiscardOutput(serial)) return 0;
#ifdef __MINGW32__
  if (!ClearCommError(serial->fileHandle, NULL, NULL)) return 0;
#else /* __MINGW32__ */
  if (!serialSetSpeed(serial, B0)) return 0;
#endif /* __MINGW32__ */
  if (!serialFlushAttributes(serial)) return 0;

  approximateDelay(500);
  if (!serialDiscardInput(serial)) return 0;

  if (!serialSetBaud(serial, baud)) return 0;
  if (!serialFlushAttributes(serial)) return 0;
  return 1;
}

FILE *
serialGetStream (SerialDevice *serial) {
  if (!serial->stream) {
#ifdef __MINGW32__
    if (serial->fileDescriptor < 0) {
      if ((serial->fileDescriptor = _open_osfhandle((long)serial->fileHandle, O_RDWR)) < 0) {
        LogError("open_osfhandle");
        return NULL;
      }
    }
#endif /* __MINGW32__ */

    if (!(serial->stream = fdopen(serial->fileDescriptor, "ab+"))) {
      LogError("fdopen");
      return NULL;
    }
  }

  return serial->stream;
}

int
serialAwaitInput (SerialDevice *serial, int timeout) {
  if (!serialFlushAttributes(serial)) return 0;

#ifdef __MINGW32__
  COMMTIMEOUTS timeouts = {MAXDWORD, 0, timeout, 0, 0};
  DWORD bytesRead;
  int ret;
  if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
    LogWindowsError("SetCommTimeouts serialAwaitInput");
    return 0;
  }

  ret = ReadFile(serial->fileHandle, NULL, 0, &bytesRead, NULL);
  if (!ret)
    LogWindowsError("ReadFile");
  return ret;
#else /* __MINGW32__ */
  return awaitInput(serial->fileDescriptor, timeout);
#endif /* __MINGW32__ */
}

int
serialReadChunk (
  SerialDevice *serial,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
) {
  if (!serialFlushAttributes(serial)) return 0;

#ifdef __MINGW32__
  COMMTIMEOUTS timeouts = {MAXDWORD, 0, initialTimeout, 0, 0};
  DWORD bytesRead;

  if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
    LogWindowsError("SetCommTimeouts serialReadChunk1");
    errno = -1;
    return 0;
  }

  if (!ReadFile(serial->fileHandle, buffer+*offset, count, &bytesRead, NULL)) {
    LogWindowsError("ReadFile");
    errno = -1;
    return 0;
  }

  if (!bytesRead) {
    errno = -2;
    return 0;
  }

  count -= bytesRead;
  offset += bytesRead;
  timeouts.ReadTotalTimeoutConstant = subsequentTimeout;
  if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
    LogWindowsError("SetCommTimeouts serialReadChunk2");
    errno = -1;
    return 0;
  }

  while (count && ReadFile(serial->fileHandle, buffer+*offset, count, &bytesRead, NULL)) {
    if (!bytesRead) {
      errno = -2;
      return 0;
    }

    count -= bytesRead;
    offset += bytesRead;
  }

  if (!count) return 1;
  LogWindowsError("ReadFile");
  errno = -1;
  return 0;
#else /* __MINGW32__ */
  return readChunk(serial->fileDescriptor, buffer, offset, count, initialTimeout, subsequentTimeout);
#endif /* __MINGW32__ */
}

ssize_t
serialReadData (
  SerialDevice *serial,
  void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
#ifdef __MINGW32__
  size_t length = 0;
  if (serialReadChunk(serial, buffer, &length, size, initialTimeout, subsequentTimeout)) return size;
  if (errno == -2) return length;
  return -1;
#else /* __MINGW32__ */
  if (!serialFlushAttributes(serial)) return -1;
  return readData(serial->fileDescriptor, buffer, size, initialTimeout, subsequentTimeout);
#endif /* __MINGW32__ */
}

ssize_t
serialWriteData (
  SerialDevice *serial,
  const void *data, size_t size
) {
  if (serialFlushAttributes(serial)) {
#ifdef __MINGW32__
    COMMTIMEOUTS timeouts = {MAXDWORD, 0, 0, 0, 15000};
    size_t left = size;
    DWORD bytesWritten;

    if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
      LogWindowsError("SetCommTimeouts serialWriteData");
      errno = -1;
      return -1;
    }

    while (left && WriteFile(serial->fileHandle, data, left, &bytesWritten, NULL)) {
      if (!bytesWritten) return size;
      left -= bytesWritten;
      data += bytesWritten;
    }

    if (!left) return size;
    LogWindowsError("WriteFile");
#else /* __MINGW32__ */
    if (writeData(serial->fileDescriptor, data, size) != -1) return size;
    LogError("serial write");
#endif /* __MINGW32__ */
  }
  return -1;
}

static int
serialGetLines (SerialDevice *serial, int *lines) {
#ifdef __MINGW32__
#define TIOCM_CTS 0
#define TIOCM_DSR 1
  /* appropriate data flow control bits need to be set for this to work */
  COMSTAT comstat;

  if (ClearCommError(serial->fileHandle,  NULL, &comstat))
    return comstat.fCtsHold | (comstat.fDsrHold < 1);

  LogWindowsError("getting modem lines");
#else /* __MINGW32__ */
  if (ioctl(serial->fileDescriptor, TIOCMGET, lines) != -1) return 1;
  LogError("TIOCMGET");
#endif /* __MINGW32__ */
  return 0;
}

static int
serialTestLines (SerialDevice *serial, int set, int clear) {
  int lines;
  if (serialGetLines(serial, &lines))
    if (((lines & set) == set) && ((~lines & clear) == clear))
      return 1;
  return 0;
}

int
serialTestLineCTS (SerialDevice *serial) {
  return serialTestLines(serial, TIOCM_CTS, 0);
}

int
serialTestLineDSR (SerialDevice *serial) {
  return serialTestLines(serial, TIOCM_DSR, 0);
}
