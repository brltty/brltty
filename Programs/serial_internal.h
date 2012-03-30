/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SERIAL_INTERNAL
#define BRLTTY_INCLUDED_SERIAL_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
#include "log.h"
#include "device.h"
#include "parse.h"
#include "timing.h"

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
  unsigned int baud;
  SerialSpeed speed;
} BaudEntry;

extern const BaudEntry serialBaudTable[];
#define BEGIN_SERIAL_BAUD_TABLE const BaudEntry serialBaudTable[] = {
#define END_SERIAL_BAUD_TABLE {0} };

extern void serialPutInitialAttributes (SerialAttributes *attributes);
extern int serialPutSpeed (SerialDevice *serial, SerialSpeed speed);
extern int serialPutDataBits (SerialAttributes *attributes, unsigned int bits);
extern int serialPutStopBits (SerialAttributes *attributes, SerialStopBits bits);
extern int serialPutParity (SerialAttributes *attributes, SerialParity parity);
extern SerialFlowControl serialPutFlowControl (SerialAttributes *attributes, SerialFlowControl flow);
extern void serialPutModemState (SerialAttributes *attributes, int enabled);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SERIAL_INTERNAL */
