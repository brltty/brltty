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

#ifdef HAVE_POSIX_THREADS
#ifdef __MINGW32__
#include "win_pthread.h"
#else /* __MINGW32__ */
#include <pthread.h>
#endif /* __MINGW32__ */
#endif /* HAVE_POSIX_THREADS */

#if defined(__MINGW32__)

#include <io.h>

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

typedef struct {
  unsigned short divisor;
  unsigned short biosBPS;
} SerialSpeed;

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

#elif defined(GRUB_RUNTIME)

#elif defined(HAVE_TERMIOS_H)

#include <termios.h>
#include <sys/ioctl.h>

#ifdef HAVE_SYS_MODEM_H
#include <sys/modem.h>
#endif /* HAVE_SYS_MODEM_H */

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
#include "log.h"
#include "device.h"
#include "parse.h"
#include "timing.h"

typedef void * (*FlowControlProc) (void *arg);

struct SerialDeviceStruct {
  int fileDescriptor;
  FILE *stream;

  SerialAttributes originalAttributes;
  SerialAttributes currentAttributes;
  SerialAttributes pendingAttributes;

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
  int pendingCharacter;
#endif /* __MINGW32__ */

#ifdef __MSDOS__
  int port;
#endif /* __MSDOS__ */
};

typedef struct {
  unsigned int baud;
  SerialSpeed speed;
} SerialBaudEntry;

extern const SerialBaudEntry *serialGetBaudEntry (unsigned int baud);
#define SERIAL_BAUD_TABLE_DECLARATION  const SerialBaudEntry serialBaudTable[]
extern SERIAL_BAUD_TABLE_DECLARATION;
#define BEGIN_SERIAL_BAUD_TABLE SERIAL_BAUD_TABLE_DECLARATION = {
#define END_SERIAL_BAUD_TABLE {0} };

extern void serialPutInitialAttributes (SerialAttributes *attributes);
extern int serialPutSpeed (SerialAttributes *attributes, SerialSpeed speed);
extern int serialPutDataBits (SerialAttributes *attributes, unsigned int bits);
extern int serialPutStopBits (SerialAttributes *attributes, SerialStopBits bits);
extern int serialPutParity (SerialAttributes *attributes, SerialParity parity);
extern SerialFlowControl serialPutFlowControl (SerialAttributes *attributes, SerialFlowControl flow);
extern int serialPutModemState (SerialAttributes *attributes, int enabled);

extern unsigned int serialGetDataBits (const SerialAttributes *attributes);
extern unsigned int serialGetStopBits (const SerialAttributes *attributes);
extern unsigned int serialGetParityBits (const SerialAttributes *attributes);

extern int serialGetAttributes (SerialDevice *serial, SerialAttributes *attributes);
extern int serialPutAttributes (SerialDevice *serial, const SerialAttributes *attributes);

extern int serialCancelInput (SerialDevice *serial);
extern int serialCancelOutput (SerialDevice *serial);

extern int serialPollInput (SerialDevice *serial, int timeout);
extern int serialDrainOutput (SerialDevice *serial);

extern int serialGetChunk (
  SerialDevice *serial,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
);

extern ssize_t serialGetData (
  SerialDevice *serial,
  void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
);

extern ssize_t serialPutData (
  SerialDevice *serial,
  const void *data, size_t size
);

extern int serialGetLines (SerialDevice *serial);
extern int serialPutLines (SerialDevice *serial, SerialLines high, SerialLines low);

extern int serialRegisterWaitLines (SerialDevice *serial, SerialLines lines);
extern int serialMonitorWaitLines (SerialDevice *serial);

extern int serialConnectDevice (SerialDevice *serial, const char *device);
extern int serialPrepareDevice (SerialDevice *serial);

extern void serialDisconnectDevice (SerialDevice *serial);
extern int serialEnsureFileDescriptor (SerialDevice *serial);
extern void serialClearError (SerialDevice *serial);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SERIAL_INTERNAL */
