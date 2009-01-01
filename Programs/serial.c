/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_MODEM_H
#include <sys/modem.h>
#endif /* HAVE_SYS_MODEM_H */

#ifdef HAVE_POSIX_THREADS
#ifdef __MINGW32__
#include "win_pthread.h"
#else /* __MINGW32__ */
#include <pthread.h>
#endif /* __MINGW32__ */
#endif /* HAVE_POSIX_THREADS */

#if defined(__MINGW32__)

#ifdef __MINGW32__
#include <io.h>
#else /* __MINGW32__ */
#include <sys/cygwin.h>
#endif /* __MINGW32__ */

typedef DWORD SerialSpeed;
typedef DCB SerialAttributes;

typedef DWORD SerialLines;
#define SERIAL_LINE_RTS 0X01
#define SERIAL_LINE_DTR 0X02
#define SERIAL_LINE_CTS MS_CTS_ON
#define SERIAL_LINE_DSR MS_DSR_ON
#define SERIAL_LINE_RNG MS_RING_ON
#define SERIAL_LINE_CAR MS_RLSD_ON

#elif defined(__MSDOS__)

#include <dos.h>
#include <dpmi.h>
#include <bios.h>
#include <go32.h>
#include <sys/farptr.h>
#include "system.h"

typedef struct {
  unsigned short divisor;
  unsigned short biosBPS;
} SerialSpeed;

#define SERIAL_DIVISOR_BASE 115200
#define SERIAL_DIVISOR(baud) (SERIAL_DIVISOR_BASE / (baud))
#define SERIAL_SPEED(baud, bios) (SerialSpeed){SERIAL_DIVISOR((baud)), (bios)}
#define SERIAL_SPEED_110    SERIAL_SPEED(   110,  0)
#define SERIAL_SPEED_150    SERIAL_SPEED(   150,  1)
#define SERIAL_SPEED_300    SERIAL_SPEED(   300,  2)
#define SERIAL_SPEED_600    SERIAL_SPEED(   600,  3)
#define SERIAL_SPEED_1200   SERIAL_SPEED(  1200,  4)
#define SERIAL_SPEED_2400   SERIAL_SPEED(  2400,  5)
#define SERIAL_SPEED_4800   SERIAL_SPEED(  4800,  6)
#define SERIAL_SPEED_9600   SERIAL_SPEED(  9600,  7)
#define SERIAL_SPEED_19200  SERIAL_SPEED( 19200,  8)
#define SERIAL_SPEED_38400  SERIAL_SPEED( 38400,  9)
#define SERIAL_SPEED_57600  SERIAL_SPEED( 57600, 10)
#define SERIAL_SPEED_115200 SERIAL_SPEED(115200, 11)

typedef union {
  unsigned char byte;
  struct {
    unsigned bits:2;
    unsigned stop:1;
    unsigned parity:2;
    unsigned bps:3;
  } fields;
} SerialBiosConfiguration;

typedef struct {
  SerialBiosConfiguration bios;
  SerialSpeed speed;
} SerialAttributes;

typedef unsigned char SerialLines;
#define SERIAL_LINE_DTR 0X01
#define SERIAL_LINE_RTS 0X02
#define SERIAL_LINE_CTS 0X10
#define SERIAL_LINE_DSR 0X20
#define SERIAL_LINE_RNG 0X40
#define SERIAL_LINE_CAR 0X80

#define SERIAL_PORT_RBR 0 /* receive buffered register */
#define SERIAL_PORT_THR 0 /* transmit holding register */
#define SERIAL_PORT_DLL 0 /* divisor latch low */
#define SERIAL_PORT_IER 1 /* interrupt enable register */
#define SERIAL_PORT_DLH 1 /* divisor latch high */
#define SERIAL_PORT_IIR 2 /* interrupt id register */
#define SERIAL_PORT_LCR 3 /* line control register */
#define SERIAL_PORT_MCR 4 /* modem control register */
#define SERIAL_PORT_MSR 6 /* modem status register */

#define SERIAL_FLAG_LCR_DLAB 0X80 /* divisor latch access bit */

#else /* UNIX */

#include <sys/ioctl.h>
#include <termios.h>

typedef speed_t SerialSpeed;
typedef struct termios SerialAttributes;

typedef int SerialLines;
#define SERIAL_LINE_RTS TIOCM_RTS
#define SERIAL_LINE_DTR TIOCM_DTR
#define SERIAL_LINE_CTS TIOCM_CTS
#define SERIAL_LINE_DSR TIOCM_DSR
#define SERIAL_LINE_RNG TIOCM_RNG
#define SERIAL_LINE_CAR TIOCM_CAR

#endif /* definitions */

#include "io_serial.h"
#include "io_misc.h"
#include "misc.h"

typedef void * (*FlowControlProc) (void *arg);

struct SerialDeviceStruct {
  int fileDescriptor;
  SerialAttributes originalAttributes;
  SerialAttributes currentAttributes;
  SerialAttributes pendingAttributes;
  FILE *stream;

  SerialLines linesState;
  SerialLines waitLines;

#ifdef HAVE_POSIX_THREADS
  FlowControlProc currentFlowControlProc;
  FlowControlProc pendingFlowControlProc;
  pthread_t flowControlThread;
  unsigned flowControlRunning:1;
#endif /* HAVE_POSIX_THREADS */

#ifdef __MINGW32__
  HANDLE fileHandle;
  int pending;
#endif /* __MINGW32__ */

#ifdef __MSDOS__
  int port;
#endif /* __MSDOS__ */
};

typedef struct {
  int baud;
  SerialSpeed speed;
} BaudEntry;
static const BaudEntry baudTable[] = {
#if defined(__MINGW32__)

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

#elif defined(__MSDOS__)

  {110, SERIAL_SPEED_110},
  {150, SERIAL_SPEED_150},
  {300, SERIAL_SPEED_300},
  {600, SERIAL_SPEED_600},
  {1200, SERIAL_SPEED_1200},
  {2400, SERIAL_SPEED_2400},
  {4800, SERIAL_SPEED_4800},
  {9600, SERIAL_SPEED_9600},
  {19200, SERIAL_SPEED_19200},
  {38400, SERIAL_SPEED_38400},
  {57600, SERIAL_SPEED_57600},
  {115200, SERIAL_SPEED_115200},

#else /* UNIX */

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

#endif /* baud table */

  {0}
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

#ifdef __MSDOS__
static unsigned short
serialPortBase (SerialDevice *serial) {
  return _farpeekw(_dos_ds, 0X0400 + 2*serial->port);
}

static unsigned char
serialReadPort (SerialDevice *serial, unsigned char port) {
  return readPort1(serialPortBase(serial)+port);
}

static void
serialWritePort (SerialDevice *serial, unsigned char port, unsigned char value) {
  writePort1(serialPortBase(serial)+port, value);
}
#endif /* __MSDOS__ */

static void
serialInitializeAttributes (SerialAttributes *attributes) {
  memset(attributes, 0, sizeof(*attributes));
#if defined(__MINGW32__)
  attributes->DCBlength = sizeof(*attributes);
  attributes->fBinary = TRUE;
  attributes->ByteSize = 8;
  attributes->BaudRate = CBR_9600;
  attributes->fRtsControl = RTS_CONTROL_ENABLE;
  attributes->fDtrControl = DTR_CONTROL_ENABLE;
  attributes->fTXContinueOnXoff = TRUE;
  attributes->XonChar = 0X11;
  attributes->XoffChar = 0X13;
#elif defined(__MSDOS__)
  attributes->speed = SERIAL_SPEED_9600;
  attributes->bios.fields.bps = attributes->speed.biosBPS;
  attributes->bios.fields.bits = 8 - 5;
#else /* UNIX */
  attributes->c_cflag = CREAD | CLOCAL | CS8;
  attributes->c_iflag = IGNPAR | IGNBRK;

#ifdef IEXTEN
  attributes->c_lflag |= IEXTEN;
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
#endif /* initialize attributes */
}

static int
serialSetSpeed (SerialDevice *serial, SerialSpeed speed) {
#if defined(__MINGW32__)
  serial->pendingAttributes.BaudRate = speed;
  return 1;
#elif defined(__MSDOS__)
  serial->pendingAttributes.speed = speed;
  serial->pendingAttributes.bios.fields.bps = serial->pendingAttributes.speed.biosBPS;
  return 1;
#else /* UNIX */
  if (cfsetospeed(&serial->pendingAttributes, speed) != -1) {
    if (cfsetispeed(&serial->pendingAttributes, speed) != -1) {
      return 1;
    } else {
      LogError("cfsetispeed");
    }
  } else {
    LogError("cfsetospeed");
  }
  return 0;
#endif /* set speed */
}

int
serialSetBaud (SerialDevice *serial, int baud) {
  const BaudEntry *entry = getBaudEntry(baud);
  if (entry) {
    if (serialSetSpeed(serial, entry->speed)) {
      return 1;
    }
  } else {
    LogPrint(LOG_WARNING, "unsupported serial baud: %d", baud);
  }
  return 0;
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

int
serialSetDataBits (SerialDevice *serial, int bits) {
#if defined(__MINGW32__) || defined(__MSDOS__)
  if ((bits < 5) || (bits > 8)) {
#else /* UNIX */
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
#endif /* test data bits */
      LogPrint(LOG_WARNING, "unsupported serial data bit count: %d", bits);
      return 0;
  }

#if defined(__MINGW32__)
  serial->pendingAttributes.ByteSize = bits;
#elif defined(__MSDOS__)
  serial->pendingAttributes.bios.fields.bits = bits - 5;
#else /* UNIX */
  serial->pendingAttributes.c_cflag &= ~CSIZE;
  serial->pendingAttributes.c_cflag |= size;
#endif /* set data bits */
  return 1;
}

int
serialSetStopBits (SerialDevice *serial, int bits) {
#if defined(__MINGW32__)
  if (bits == 1) {
    serial->pendingAttributes.StopBits = ONESTOPBIT;
  } else if (bits == 15) {
    serial->pendingAttributes.StopBits = ONE5STOPBITS;
  } else if (bits == 2) {
    serial->pendingAttributes.StopBits = TWOSTOPBITS;
#elif defined(__MSDOS__)
  if (bits == 1) {
    serial->pendingAttributes.bios.fields.stop = 0;
  } else if (bits == 15 || bits == 2) {
    serial->pendingAttributes.bios.fields.stop = 1;
#else /* UNIX */
  if (bits == 1) {
    serial->pendingAttributes.c_cflag &= ~CSTOPB;
  } else if (bits == 2) {
    serial->pendingAttributes.c_cflag |= CSTOPB;
#endif /* set stop bits */
  } else {
    LogPrint(LOG_WARNING, "unsupported serial stop bit count: %d", bits);
    return 0;
  }
  return 1;
}

int
serialSetParity (SerialDevice *serial, SerialParity parity) {
#if defined(__MINGW32__)
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
        goto unsupportedParity;
    }

    serial->pendingAttributes.fParity = TRUE;
  }
#elif defined(__MSDOS__)
  switch (parity) {
    case SERIAL_PARITY_NONE:
      serial->pendingAttributes.bios.fields.parity = 0;
      break;

    case SERIAL_PARITY_ODD:
      serial->pendingAttributes.bios.fields.parity = 1;
      break;

    case SERIAL_PARITY_EVEN:
      serial->pendingAttributes.bios.fields.parity = 2;
      break;

    default:
      goto unsupportedParity;
  }
#else /* UNIX */
  serial->pendingAttributes.c_cflag &= ~(PARENB | PARODD);

#ifdef PARSTK
  serial->pendingAttributes.c_cflag &= ~PARSTK;
#endif /* PARSTK */

  if (parity != SERIAL_PARITY_NONE) {
    if (parity == SERIAL_PARITY_ODD) {
      serial->pendingAttributes.c_cflag |= PARODD;
    } else

#ifdef PARSTK
    if (parity == SERIAL_PARITY_SPACE) {
      serial->pendingAttributes.c_cflag |= PARSTK;
    } else

    if (parity == SERIAL_PARITY_MARK) {
      serial->pendingAttributes.c_cflag |= PARSTK | PARODD;
    } else
#endif /* PARSTK */

    if (parity != SERIAL_PARITY_EVEN) goto unsupportedParity;
    serial->pendingAttributes.c_cflag |= PARENB;
  }
#endif /* set parity */
  return 1;

unsupportedParity:
  LogPrint(LOG_WARNING, "unsupported serial parity: %d", parity);
  return 0;
}

#ifdef HAVE_POSIX_THREADS
static void *
flowControlProc_InputCts (void *arg) {
  SerialDevice *serial = arg;
  int up = serialTestLineCTS(serial);

  while (1) {
    serialSetLineRTS(serial, up);
    serialWaitLineCTS(serial, (up = !up), 0);
  }
}

static int
serialStartFlowControlThread (SerialDevice *serial) {
  if (!serial->flowControlRunning && serial->currentFlowControlProc) {
    pthread_t thread;
    pthread_attr_t attributes;

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attributes, serial->currentFlowControlProc, serial)) {
      LogError("pthread_create");
      return 0;
    }

    serial->flowControlThread = thread;
    serial->flowControlRunning = 1;
  }

  return 1;
}

static void
serialStopFlowControlThread (SerialDevice *serial) {
  if (serial->flowControlRunning) {
    pthread_cancel(serial->flowControlThread);
    serial->flowControlRunning = 0;
  }
}
#endif /* HAVE_POSIX_THREADS */

int
serialSetFlowControl (SerialDevice *serial, SerialFlowControl flow) {
#if defined(__MINGW32__)
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
#elif defined(__MSDOS__)
  /* no supported flow control */
#else /* UNIX */
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
#endif /* set flow control */

#ifdef HAVE_POSIX_THREADS
  if (flow & SERIAL_FLOW_INPUT_CTS) {
    flow &= ~SERIAL_FLOW_INPUT_CTS;
    serial->pendingFlowControlProc = flowControlProc_InputCts;
  } else {
    serial->pendingFlowControlProc = NULL;
  }

#if defined(__MINGW32__)
#elif defined(__MSDOS__)
#else /* UNIX */
  if (serial->pendingFlowControlProc) {
    serial->pendingAttributes.c_cflag &= ~CLOCAL;
  } else {
    serial->pendingAttributes.c_cflag |= CLOCAL;
  }
#endif /* adjust attributes */
#endif /* HAVE_POSIX_THREADS */

  if (!flow) return 1;
  LogPrint(LOG_WARNING, "unsupported serial flow control: 0X%02X", flow);
  return 0;
}

int
serialGetCharacterBits (SerialDevice *serial) {
  int bits = 2; /* 1 start bit + 1 stop bit */

#if defined(__MINGW32__)
  bits += serial->pendingAttributes.ByteSize;
  if (serial->pendingAttributes.fParity && (serial->pendingAttributes.Parity != NOPARITY)) ++bits;
  if (serial->pendingAttributes.StopBits == TWOSTOPBITS) {
    ++bits;
  } else if (serial->pendingAttributes.StopBits != ONESTOPBIT) {
    LogPrint(LOG_WARNING, "unsupported serial stop bits value: %X", serial->pendingAttributes.StopBits);
  }
#elif defined(__MSDOS__)
  bits += serial->pendingAttributes.bios.fields.bits + 5;
  if (serial->pendingAttributes.bios.fields.parity) ++bits;
  if (serial->pendingAttributes.bios.fields.stop) ++bits;
#else /* UNIX */
  {
    tcflag_t size = serial->pendingAttributes.c_cflag & CSIZE;
    switch (size) {
  #ifdef CS5
      case CS5: bits += 5; break;
  #endif /* CS5 */

  #ifdef CS6
      case CS6: bits += 6; break;
  #endif /* CS6 */

  #ifdef CS7
      case CS7: bits += 7; break;
  #endif /* CS7 */

  #ifdef CS8
      case CS8: bits += 8; break;
  #endif /* CS8 */

      default:
        LogPrint(LOG_WARNING, "unsupported serial data bits value: %lX", (unsigned long) size);
        break;
    }
  }

  if (serial->pendingAttributes.c_cflag & PARENB) ++bits;
  if (serial->pendingAttributes.c_cflag & CSTOPB) ++bits;
#endif /* increment bits */

  return bits;
}

int
serialDiscardInput (SerialDevice *serial) {
#if defined(__MINGW32__)
  if (PurgeComm(serial->fileHandle, PURGE_RXCLEAR)) return 1;
  LogWindowsError("PurgeComm");
#elif defined(__MSDOS__)
  LogPrint(LOG_DEBUG, "function not supported: %s", __func__);
  return 1;
#else /* UNIX */
  if (tcflush(serial->fileDescriptor, TCIFLUSH) != -1) return 1;
  if (errno == EINVAL) return 1;
  LogError("TCIFLUSH");
#endif /* discard input */
  return 0;
}

int
serialDiscardOutput (SerialDevice *serial) {
#if defined(__MINGW32__)
  if (PurgeComm(serial->fileHandle, PURGE_TXCLEAR)) return 1;
  LogWindowsError("PurgeComm");
#elif defined(__MSDOS__)
  LogPrint(LOG_DEBUG, "function not supported: %s", __func__);
  return 1;
#else /* UNIX */
  if (tcflush(serial->fileDescriptor, TCOFLUSH) != -1) return 1;
  if (errno == EINVAL) return 1;
  LogError("TCOFLUSH");
#endif /* discard output */
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

#if defined(__MINGW32__)
  if (FlushFileBuffers(serial->fileHandle)) return 1;
  LogWindowsError("FlushFileBuffers");
#elif defined(__MSDOS__)
  LogPrint(LOG_DEBUG, "function not supported: %s", __func__);
  return 1;
#else /* UNIX */
  do {
    if (tcdrain(serial->fileDescriptor) != -1) return 1;
  } while (errno == EINTR);
  LogError("tcdrain");
#endif /* drain output */
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
#if defined(__MINGW32__)
  serial->currentAttributes.DCBlength = sizeof(serial->currentAttributes);
  if (GetCommState(serial->fileHandle, &serial->currentAttributes)) return 1;
  LogWindowsError("GetCommState");
#elif defined(__MSDOS__)
  int interruptsWereEnabled = disable();
  unsigned char lcr = serialReadPort(serial, SERIAL_PORT_LCR);
  int divisor;

  serialWritePort(serial, SERIAL_PORT_LCR,
                  lcr | SERIAL_FLAG_LCR_DLAB);
  divisor = (serialReadPort(serial, SERIAL_PORT_DLH) << 8) |
            serialReadPort(serial, SERIAL_PORT_DLL);
  serialWritePort(serial, SERIAL_PORT_LCR, lcr);
  if (interruptsWereEnabled) enable();

  serial->currentAttributes.bios.byte = lcr;
  {
    const BaudEntry *baud = getBaudEntry(SERIAL_DIVISOR_BASE/divisor);
    if (baud) {
      serial->currentAttributes.speed = baud->speed;
    } else {
      memset(&serial->currentAttributes.speed, 0,
             sizeof(serial->currentAttributes.speed));
    }
  }
  serial->currentAttributes.bios.fields.bps = serial->currentAttributes.speed.biosBPS;

  return 1;
#else /* UNIX */
  if (tcgetattr(serial->fileDescriptor, &serial->currentAttributes) != -1) return 1;
  LogError("tcgetattr");
#endif /* read attributes */
  return 0;
}

static int
serialWriteAttributes (SerialDevice *serial, const SerialAttributes *attributes) {
  if (!serialCompareAttributes(attributes, &serial->currentAttributes)) {
    if (!serialDrainOutput(serial)) return 0;

#if defined(__MINGW32__)
    if (!SetCommState(serial->fileHandle, (SerialAttributes *)attributes)) {
      LogWindowsError("SetCommState");
      return 0;
    }
#elif defined(__MSDOS__)
    {
      if (attributes->speed.biosBPS <= 7) {
        if (bioscom(0, attributes->bios.byte, serial->port) & 0X0700) {
          LogPrint(LOG_ERR, "serialWriteAttributes failed");
          return 0;
        }
      } else {
        int interruptsWereEnabled = disable();
        SerialBiosConfiguration lcr = attributes->bios;
        lcr.fields.bps = 0;

        serialWritePort(serial, SERIAL_PORT_LCR,
                        lcr.byte | SERIAL_FLAG_LCR_DLAB);
        serialWritePort(serial, SERIAL_PORT_DLL,
                        attributes->speed.divisor & 0XFF);
        serialWritePort(serial, SERIAL_PORT_DLH,
                        attributes->speed.divisor >> 8);
        serialWritePort(serial, SERIAL_PORT_LCR, lcr.byte);
        if (interruptsWereEnabled) enable();
      }
    }
#else /* UNIX */
    if (tcsetattr(serial->fileDescriptor, TCSANOW, attributes) == -1) {
      LogError("tcsetattr");
      return 0;
    }
#endif /* write attributes */

    serialCopyAttributes(&serial->currentAttributes, attributes);
  }
  return 1;
}

static int
serialFlushAttributes (SerialDevice *serial) {
#ifdef HAVE_POSIX_THREADS
  int restartFlowControlThread = serial->pendingFlowControlProc != serial->currentFlowControlProc;
  if (restartFlowControlThread) serialStopFlowControlThread(serial);
#endif /* HAVE_POSIX_THREADS */

  if (!serialWriteAttributes(serial, &serial->pendingAttributes)) return 0;

#ifdef HAVE_POSIX_THREADS
  if (restartFlowControlThread) {
    serial->currentFlowControlProc = serial->pendingFlowControlProc;
    if (!serialStartFlowControlThread(serial)) return 0;
  }
#endif /* HAVE_POSIX_THREADS */

  return 1;
}

int
serialAwaitInput (SerialDevice *serial, int timeout) {
  if (!serialFlushAttributes(serial)) return 0;

#ifdef __MINGW32__
  if (serial->pending != -1) return 1;

  {
    COMMTIMEOUTS timeouts = {MAXDWORD, 0, timeout, 0, 0};
    DWORD bytesRead;
    char c;

    if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
      LogWindowsError("SetCommTimeouts serialAwaitInput");
      setSystemErrno();
      return 0;
    }

    if (!ReadFile(serial->fileHandle, &c, 1, &bytesRead, NULL)) {
      LogWindowsError("ReadFile");
      setSystemErrno();
      return 0;
    }

    if (bytesRead) {
      serial->pending = (unsigned char)c;
      return 1;
    }
  }
  errno = EAGAIN;

  return 0;
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

  if (serial->pending != -1) {
    * (unsigned char *) buffer = serial->pending;
    serial->pending = -1;
    bytesRead = 1;
  } else {
    if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
      LogWindowsError("SetCommTimeouts serialReadChunk1");
      setSystemErrno();
      return 0;
    }

    if (!ReadFile(serial->fileHandle, buffer+*offset, count, &bytesRead, NULL)) {
      LogWindowsError("ReadFile");
      setSystemErrno();
      return 0;
    }

    if (!bytesRead) {
      errno = EAGAIN;
      return 0;
    }
  }

  count -= bytesRead;
  *offset += bytesRead;
  timeouts.ReadTotalTimeoutConstant = subsequentTimeout;
  if (!(SetCommTimeouts(serial->fileHandle, &timeouts))) {
    LogWindowsError("SetCommTimeouts serialReadChunk2");
    setSystemErrno();
    return 0;
  }

  while (count && ReadFile(serial->fileHandle, buffer+*offset, count, &bytesRead, NULL)) {
    if (!bytesRead) {
      errno = EAGAIN;
      return 0;
    }

    count -= bytesRead;
    *offset += bytesRead;
  }

  if (!count) return 1;
  LogWindowsError("ReadFile");
  setSystemErrno();
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
  if (errno == EAGAIN) return length;
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
      setSystemErrno();
      return -1;
    }

    while (left && WriteFile(serial->fileHandle, data, left, &bytesWritten, NULL)) {
      if (!bytesWritten) break;
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
serialGetLines (SerialDevice *serial, SerialLines *lines) {
#if defined(__MINGW32__)
  DCB dcb;
  if (!GetCommModemStatus(serial->fileHandle, &serial->linesState)) {
    LogWindowsError("GetCommModemStatus");
    return 0;
  }
  dcb.DCBlength = sizeof(dcb);
  if (!GetCommState(serial->fileHandle, &dcb)) {
    LogWindowsError("GetCommState");
    return 0;
  }
  if (dcb.fRtsControl == RTS_CONTROL_ENABLE)
    serial->linesState |= SERIAL_LINE_RTS;
  if (dcb.fDtrControl == DTR_CONTROL_ENABLE)
    serial->linesState |= SERIAL_LINE_DTR;
#elif defined(__MSDOS__)
  serial->linesState = serialReadPort(serial, SERIAL_PORT_MSR) & 0XF0;
#elif defined(TIOCMGET)
  if (ioctl(serial->fileDescriptor, TIOCMGET, &serial->linesState) == -1) {
    LogError("TIOCMGET");
    return 0;
  }
#else /* get lines */
#warning getting modem lines not supported on this platform
  serial->linesState = SERIAL_LINE_RTS | SERIAL_LINE_CTS | SERIAL_LINE_DTR | SERIAL_LINE_DSR | SERIAL_LINE_CAR;
#endif /* get lines */

  *lines = serial->linesState;
  return 1;
}

static int
serialSetLines (SerialDevice *serial, SerialLines high, SerialLines low) {
#if defined(__MINGW32__)
  DCB dcb;
  dcb.DCBlength = sizeof(dcb);
  if (GetCommState(serial->fileHandle, &dcb)) {
    if (low & SERIAL_LINE_RTS)
      dcb.fRtsControl = RTS_CONTROL_DISABLE;
    else if (high & SERIAL_LINE_RTS)
      dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (low & SERIAL_LINE_DTR)
      dcb.fDtrControl = DTR_CONTROL_DISABLE;
    else if (high & SERIAL_LINE_DTR)
      dcb.fDtrControl = DTR_CONTROL_ENABLE;

    if (SetCommState(serial->fileHandle, &dcb)) return 1;
    LogWindowsError("SetCommState");
  } else {
    LogWindowsError("GetCommState");
  }
#elif defined(__MSDOS__)
  int interruptsWereEnabled = disable();
  unsigned char oldMCR = serialReadPort(serial, SERIAL_PORT_MCR);

  serialWritePort(serial, SERIAL_PORT_MCR,
                  (oldMCR | high) & ~low);
  if (interruptsWereEnabled) enable();
  return 1;
#elif defined(TIOCMSET)
  int status;
  if (serialGetLines(serial, &status) != -1) {
    status |= high;
    status &= ~low;
    if (ioctl(serial->fileDescriptor, TIOCMSET, &status) != -1) return 1;
    LogError("TIOCMSET");
  }
#else /* set lines */
#warning setting modem lines not supported on this platform
#endif /* set lines */
  return 0;
}

static int
serialSetLine (SerialDevice *serial, SerialLines line, int up) {
  return serialSetLines(serial, up?line:0, up?0:line);
}

int
serialSetLineRTS (SerialDevice *serial, int up) {
  return serialSetLine(serial, SERIAL_LINE_RTS, up);
}

int
serialSetLineDTR (SerialDevice *serial, int up) {
  return serialSetLine(serial, SERIAL_LINE_DTR, up);
}

static int
serialTestLines (SerialDevice *serial, SerialLines high, SerialLines low) {
  SerialLines lines;
  if (serialGetLines(serial, &lines))
    if (((lines & high) == high) && ((~lines & low) == low))
      return 1;
  return 0;
}

int
serialTestLineCTS (SerialDevice *serial) {
  return serialTestLines(serial, SERIAL_LINE_CTS, 0);
}

int
serialTestLineDSR (SerialDevice *serial) {
  return serialTestLines(serial, SERIAL_LINE_DSR, 0);
}

static int
serialDefineWaitLines (SerialDevice *serial, SerialLines lines) {
  if (lines != serial->waitLines) {
#ifdef __MINGW32__
    DWORD eventMask = 0;

    if (lines & SERIAL_LINE_CTS) eventMask |= EV_CTS;
    if (lines & SERIAL_LINE_DSR) eventMask |= EV_DSR;
    if (lines & SERIAL_LINE_RNG) eventMask |= EV_RING;
    if (lines & SERIAL_LINE_CAR) eventMask |= EV_RLSD;

    if (!SetCommMask(serial->fileHandle, eventMask)) {
      LogWindowsError("SetCommMask");
      return 0;
    }
#endif /* __MINGW32__ */

    serial->waitLines = lines;
  }

  return 1;
}

static int
serialMonitorWaitLines (SerialDevice *serial) {
#if defined(__MINGW32__)
  DWORD event;
  if (WaitCommEvent(serial->fileHandle, &event, NULL)) return 1;
  LogWindowsError("WaitCommEvent");
#elif defined(TIOCMIWAIT)
  if (ioctl(serial->fileDescriptor, TIOCMIWAIT, serial->waitLines) != -1) return 1;
  LogError("TIOCMIWAIT");
#else
  SerialLines old = serial->linesState & serial->waitLines;
  SerialLines new;

  while (serialGetLines(serial, &new))
    if ((new & serial->waitLines) != old)
      return 1;
#endif
  return 0;
}

static int
serialWaitLines (SerialDevice *serial, SerialLines high, SerialLines low) {
  SerialLines lines = high | low;
  int ok = 0;

  if (serialDefineWaitLines(serial, lines)) {
    while (!serialTestLines(serial, high, low))
      if (!serialMonitorWaitLines(serial))
        goto done;
    ok = 1;
  }

done:
  serialDefineWaitLines(serial, 0);
  return ok;
}

static int
serialWaitFlank (SerialDevice *serial, SerialLines line, int up) {
  int ok = 0;

  if (serialDefineWaitLines(serial, line)) {
    while (!serialTestLines(serial, up?0:line, up?line:0))
      if (!serialMonitorWaitLines(serial))
        goto done;
    if (serialMonitorWaitLines(serial)) ok = 1;
  }

done:
  serialDefineWaitLines(serial, 0);
  return ok;
}

int
serialWaitLine (SerialDevice *serial, SerialLines line, int up, int flank) {
  return flank? serialWaitFlank(serial, line, up):
                serialWaitLines(serial, up?line:0, up?0:line);
}

int
serialWaitLineCTS (SerialDevice *serial, int up, int flank) {
  return serialWaitLine(serial, SERIAL_LINE_CTS, up, flank);
}

int
serialWaitLineDSR (SerialDevice *serial, int up, int flank) {
  return serialWaitLine(serial, SERIAL_LINE_DSR, up, flank);
}

SerialDevice *
serialOpenDevice (const char *path) {
  SerialDevice *serial;
  if ((serial = malloc(sizeof(*serial)))) {
    char *device;
    if ((device = getDevicePath(path))) {
#ifdef __MINGW32__
      if ((serial->fileHandle = CreateFile(device, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE) {
        serial->fileDescriptor = -1;
#else /* __MINGW32__ */
      if ((serial->fileDescriptor = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK)) != -1) {
#ifdef __MSDOS__
        char *truePath, *com;

        if ((truePath = _truename(path, NULL)) &&
            (com = strstr(truePath,"COM"))) {
          serial->port = atoi(com+3) - 1;
#else /* __MSDOS__ */
        if (isatty(serial->fileDescriptor)) {
#endif /* __MSDOS__ */
#endif /* __MINGW32__ */

          if (serialReadAttributes(serial)) {
            serialCopyAttributes(&serial->originalAttributes, &serial->currentAttributes);
            serialInitializeAttributes(&serial->pendingAttributes);

            serial->stream = NULL;

            serial->linesState = 0;
            serial->waitLines = 0;

#ifdef HAVE_POSIX_THREADS
            serial->currentFlowControlProc = NULL;
            serial->pendingFlowControlProc = NULL;
            serial->flowControlRunning = 0;
#endif /* HAVE_POSIX_THREADS */

#ifdef __MINGW32__
            serial->pending = -1;
#endif /* __MINGW32__ */

            LogPrint(LOG_DEBUG, "serial device opened: %s: fd=%d",
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
#ifdef __MSDOS__
        } else {
          LogPrint(LOG_ERR, "could not determine serial device port number: %s", device);
        }

        if (truePath) free(truePath);
#else /* __MSDOS__ */
        } else {
          LogPrint(LOG_ERR, "not a serial device: %s", device);
        }
#endif /* __MSDOS__ */

        close(serial->fileDescriptor);
#endif /* __MINGW32__ */
      } else {
        LogPrint(LOG_ERR, "cannot open serial device: %s: %s", device, strerror(errno));
      }

      free(device);
    }

    free(serial);
  } else {
    LogError("malloc");
  }

  return NULL;
}

void
serialCloseDevice (SerialDevice *serial) {
#ifdef HAVE_POSIX_THREADS
  serialStopFlowControlThread(serial);
#endif /* HAVE_POSIX_THREADS */

  serialWriteAttributes(serial, &serial->originalAttributes);

  if (serial->stream) {
    fclose(serial->stream);
  }

#ifdef __MINGW32__
  else if (serial->fileDescriptor < 0) {
    CloseHandle(serial->fileHandle);
  }
#endif /* __MINGW32__ */

  else {
    close(serial->fileDescriptor);
  }

  free(serial);
}

int
serialRestartDevice (SerialDevice *serial, int baud) {
  SerialLines highLines = 0;
  SerialLines lowLines = 0;
  int usingB0;

#ifdef HAVE_POSIX_THREADS
  FlowControlProc flowControlProc = serial->pendingFlowControlProc;
#endif /* HAVE_POSIX_THREADS */

#ifdef __MINGW32__
  if (!ClearCommError(serial->fileHandle, NULL, NULL)) return 0;
#endif /* __MINGW32__ */

  if (serial->stream) clearerr(serial->stream);
  if (!serialDiscardOutput(serial)) return 0;

#ifdef HAVE_POSIX_THREADS
  serial->pendingFlowControlProc = NULL;
#endif /* HAVE_POSIX_THREADS */

#ifdef B0
  if (!serialSetSpeed(serial, B0)) return 0;
  usingB0 = 1;
#else /* B0 */
  usingB0 = 0;
#endif /* B0 */

  if (!serialFlushAttributes(serial)) {
    if (!usingB0) return 0;
    if (!serialSetBaud(serial, baud)) return 0;
    if (!serialFlushAttributes(serial)) return 0;
    usingB0 = 0;
  }

  if (!usingB0) {
    SerialLines lines;
    if (!serialGetLines(serial, &lines)) return 0;

    {
      static const SerialLines linesTable[] = {SERIAL_LINE_DTR, SERIAL_LINE_RTS, 0};
      const SerialLines *line = linesTable;
      while (*line) {
        *((lines & *line)? &highLines: &lowLines) |= *line;
        line++;
      }
    }

    if (highLines)
      if (!serialSetLines(serial, 0, highLines|lowLines))
        return 0;
  }

  approximateDelay(500);
  if (!serialDiscardInput(serial)) return 0;

  if (!usingB0)
    if (!serialSetLines(serial, highLines, lowLines))
      return 0;

#ifdef HAVE_POSIX_THREADS
  serial->pendingFlowControlProc = flowControlProc;
#endif /* HAVE_POSIX_THREADS */

  if (!serialSetBaud(serial, baud)) return 0;
  if (!serialFlushAttributes(serial)) return 0;
  return 1;
}

FILE *
serialGetStream (SerialDevice *serial) {
  if (!serial->stream) {
#ifdef __MINGW32__
    if (serial->fileDescriptor < 0) {
#ifdef __CYGWIN32__
      if ((serial->fileDescriptor = cygwin_attach_handle_to_fd("serialdevice", -1, serial->fileHandle, TRUE, GENERIC_READ|GENERIC_WRITE)) < 0) {
        LogError("cygwin_attach_handle_to_fd");
#else /* __CYGWIN32__ */
      if ((serial->fileDescriptor = _open_osfhandle((long)serial->fileHandle, O_RDWR)) < 0) {
        LogError("open_osfhandle");
#endif /* __CYGWIN32__ */
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
isSerialDevice (const char **path) {
#ifdef ALLOW_DOS_DEVICE_NAMES
  if (isDosDevice(*path, "COM")) return 1;
#endif /* ALLOW_DOS_DEVICE_NAMES */

  if (!isQualifiedDevice(path, "serial"))
    if (isQualifiedDevice(path, NULL))
      return 0;

  if (!**path) *path = FIRST_SERIAL_DEVICE;
  return 1;
}
