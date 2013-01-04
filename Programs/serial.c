/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>

#include "log.h"
#include "parse.h"
#include "device.h"
#include "timing.h"

#if defined(USE_SERIAL_PACKAGE_NONE)
#include "serial_none.h"
#elif defined(USE_SERIAL_PACKAGE_GRUB)
#include "serial_grub.h"
#elif defined(USE_SERIAL_PACKAGE_MSDOS)
#include "serial_msdos.h"
#elif defined(USE_SERIAL_PACKAGE_TERMIOS)
#include "serial_termios.h"
#elif defined(USE_SERIAL_PACKAGE_WINDOWS)
#include "serial_windows.h"
#else /* serial package */
#error serial package not selected
#include "serial_none.h"
#endif /* serial package */

#include "serial_internal.h"

const SerialBaudEntry *
serialGetBaudEntry (unsigned int baud) {
  const SerialBaudEntry *entry = serialBaudTable;
  while (entry->baud) {
    if (baud == entry->baud) return entry;
    ++entry;
  }
  return NULL;
}

static void
serialInitializeAttributes (SerialAttributes *attributes) {
  memset(attributes, 0, sizeof(*attributes));
  serialPutInitialAttributes(attributes);

  {
    const SerialBaudEntry *entry = serialGetBaudEntry(SERIAL_DEFAULT_BAUD);

    if (!entry) {
      logMessage(LOG_WARNING, "default serial baud not defined: %u", SERIAL_DEFAULT_BAUD);
    } else if (!serialPutSpeed(attributes, entry->speed)) {
      logMessage(LOG_WARNING, "default serial baud not supported: %u", SERIAL_DEFAULT_BAUD);
    }
  }

  if (!serialPutDataBits(attributes, SERIAL_DEFAULT_DATA_BITS)) {
    logMessage(LOG_WARNING, "default serial data bits not supported: %u", SERIAL_DEFAULT_DATA_BITS);
  }

  if (!serialPutStopBits(attributes, SERIAL_DEFAULT_STOP_BITS)) {
    logMessage(LOG_WARNING, "default serial stop bits not supported: %u", SERIAL_DEFAULT_STOP_BITS);
  }

  if (!serialPutParity(attributes, SERIAL_DEFAULT_PARITY)) {
    logMessage(LOG_WARNING, "default serial parity not supported: %u", SERIAL_DEFAULT_PARITY);
  }

  if (serialPutFlowControl(attributes, SERIAL_DEFAULT_FLOW_CONTROL)) {
    logMessage(LOG_WARNING, "default serial flow control not supported: 0X%04X", SERIAL_DEFAULT_FLOW_CONTROL);
  }

  {
    int state = 0;

    if (!serialPutModemState(attributes, state)) {
      logMessage(LOG_WARNING, "default serial modem state not supported: %d", state);
    }
  }
}

int
serialSetBaud (SerialDevice *serial, unsigned int baud) {
  const SerialBaudEntry *entry = serialGetBaudEntry(baud);

  if (entry) {
    if (serialPutSpeed(&serial->pendingAttributes, entry->speed)) {
      return 1;
    } else {
      logMessage(LOG_WARNING, "unsupported serial baud: %d", baud);
    }
  } else {
    logMessage(LOG_WARNING, "undefined serial baud: %d", baud);
  }

  return 0;
}

int
serialValidateBaud (unsigned int *baud, const char *description, const char *word, const unsigned int *choices) {
  if (!*word || isUnsignedInteger(baud, word)) {
    const SerialBaudEntry *entry = serialGetBaudEntry(*baud);

    if (entry) {
      if (!choices) return 1;

      while (*choices) {
        if (*baud == *choices) return 1;
        choices += 1;
      }

      logMessage(LOG_ERR, "unsupported %s: %u", description, *baud);
      return 0;
    } else {
      logMessage(LOG_ERR, "undefined %s: %u", description, *baud);
    }
  } else {
    logMessage(LOG_ERR, "invalid %s: %u", description, *baud);
  }

  return 0;
}

int
serialSetDataBits (SerialDevice *serial, unsigned int bits) {
  if (serialPutDataBits(&serial->pendingAttributes, bits)) return 1;
  logMessage(LOG_WARNING, "unsupported serial data bit count: %d", bits);
  return 0;
}

int
serialSetStopBits (SerialDevice *serial, SerialStopBits bits) {
  if (serialPutStopBits(&serial->pendingAttributes, bits)) return 1;
  logMessage(LOG_WARNING, "unsupported serial stop bit count: %d", bits);
  return 0;
}

int
serialSetParity (SerialDevice *serial, SerialParity parity) {
  if (serialPutParity(&serial->pendingAttributes, parity)) return 1;
  logMessage(LOG_WARNING, "unsupported serial parity: %d", parity);
  return 0;
}

#ifdef HAVE_POSIX_THREADS
static void *
flowControlProc_InputCts (void *arg) {
  SerialDevice *serial = arg;
  int up = serialTestLineCTS(serial);

  while (!serial->flowControlStop) {
    serialSetLineRTS(serial, up);
    serialWaitLineCTS(serial, (up = !up), 0);
  }

  return NULL;
}

static int
serialStartFlowControlThread (SerialDevice *serial) {
  if (!serial->flowControlRunning && serial->currentFlowControlProc) {
    pthread_t thread;
    pthread_attr_t attributes;

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

    serial->flowControlStop = 0;
    if (pthread_create(&thread, &attributes, serial->currentFlowControlProc, serial)) {
      logSystemError("pthread_create");
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
    serial->flowControlStop = 1;
    serial->flowControlRunning = 0;
  }
}
#endif /* HAVE_POSIX_THREADS */

int
serialSetFlowControl (SerialDevice *serial, SerialFlowControl flow) {
  flow = serialPutFlowControl(&serial->pendingAttributes, flow);

#ifdef HAVE_POSIX_THREADS
  if (flow & SERIAL_FLOW_INPUT_CTS) {
    flow &= ~SERIAL_FLOW_INPUT_CTS;
    serial->pendingFlowControlProc = flowControlProc_InputCts;
  } else {
    serial->pendingFlowControlProc = NULL;
  }

  {
    int state = !!serial->pendingFlowControlProc;

    if (!serialPutModemState(&serial->pendingAttributes, state)) {
      logMessage(LOG_WARNING, "unsupported serial modem state: %d", state);
    }
  }
#endif /* HAVE_POSIX_THREADS */

  if (!flow) return 1;
  logMessage(LOG_WARNING, "unsupported serial flow control: 0X%02X", flow);
  return 0;
}

int
serialSetParameters (SerialDevice *serial, const SerialParameters *parameters) {
  if (!serialSetBaud(serial, parameters->baud)) return 0;
  if (!serialSetDataBits(serial, parameters->dataBits)) return 0;
  if (!serialSetStopBits(serial, parameters->stopBits)) return 0;
  if (!serialSetParity(serial, parameters->parity)) return 0;
  if (!serialSetFlowControl(serial, parameters->flowControl)) return 0;
  return 1;
}

unsigned int
serialGetCharacterSize (const SerialParameters *parameters) {
  unsigned int size = 1 /* start bit */ + parameters->dataBits;
  size += (parameters->stopBits == SERIAL_STOP_1)? 1: 2;
  if (parameters->parity != SERIAL_PARITY_NONE) size += 1;
  return size;
}

unsigned int
serialGetCharacterBits (SerialDevice *serial) {
  const SerialAttributes *attributes = &serial->pendingAttributes;
  return 1 /* start bit */
       + serialGetDataBits(attributes)
       + serialGetParityBits(attributes)
       + serialGetStopBits(attributes)
       ;
}

int
serialDiscardInput (SerialDevice *serial) {
  return serialCancelInput(serial);
}

int
serialDiscardOutput (SerialDevice *serial) {
  return serialCancelOutput(serial);
}

int
serialFlushOutput (SerialDevice *serial) {
  if (serial->stream) {
    if (fflush(serial->stream) == EOF) {
      logSystemError("fflush");
      return 0;
    }
  }
  return 1;
}

int
serialAwaitOutput (SerialDevice *serial) {
  if (!serialFlushOutput(serial)) return 0;
  if (!serialDrainOutput(serial)) return 0;
  return 1;
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
  return serialGetAttributes(serial, &serial->currentAttributes);
}

static int
serialWriteAttributes (SerialDevice *serial, const SerialAttributes *attributes) {
  if (!serialCompareAttributes(attributes, &serial->currentAttributes)) {
    if (!serialAwaitOutput(serial)) return 0;
    if (!serialPutAttributes(serial, attributes)) return 0;
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
  if (!serialPollInput(serial, timeout)) return 0;
  return 1;
}

ssize_t
serialReadData (
  SerialDevice *serial,
  void *buffer, size_t size,
  int initialTimeout, int subsequentTimeout
) {
  if (!serialFlushAttributes(serial)) return -1;
  return serialGetData(serial, buffer, size, initialTimeout, subsequentTimeout);
}

int
serialReadChunk (
  SerialDevice *serial,
  void *buffer, size_t *offset, size_t count,
  int initialTimeout, int subsequentTimeout
) {
  unsigned char *byte = buffer;
  const unsigned char *const first = byte;
  const unsigned char *const end = first + count;
  int timeout = *offset? subsequentTimeout: initialTimeout;

  if (!serialFlushAttributes(serial)) return 0;
  byte += *offset;

  while (byte < end) {
    ssize_t result = serialGetData(serial, byte, 1, timeout, subsequentTimeout);

    if (!result) {
      result = -1;
      errno = EAGAIN;
    }

    if (result == -1) {
      if (errno == EINTR) continue;
      return 0;
    }

    byte += 1;
    *offset += 1;
    timeout = subsequentTimeout;
  }

  return 1;
}

ssize_t
serialWriteData (
  SerialDevice *serial,
  const void *data, size_t size
) {
  if (!serialFlushAttributes(serial)) return -1;
  return serialPutData(serial, data, size);
}

static int
serialReadLines (SerialDevice *serial, SerialLines *lines) {
  int result = serialGetLines(serial);
  if (result) *lines = serial->linesState;
  return result;
}

static int
serialWriteLines (SerialDevice *serial, SerialLines high, SerialLines low) {
  return serialPutLines(serial, high, low);
}

static int
serialSetLine (SerialDevice *serial, SerialLines line, int up) {
  return serialWriteLines(serial, up?line:0, up?0:line);
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
  if (serialReadLines(serial, &lines))
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
    if (!serialRegisterWaitLines(serial, lines)) return 0;
    serial->waitLines = lines;
  }

  return 1;
}

static int
serialAwaitLineChange (SerialDevice *serial) {
  return serialMonitorWaitLines(serial);
}

static int
serialWaitLines (SerialDevice *serial, SerialLines high, SerialLines low) {
  SerialLines lines = high | low;
  int ok = 0;

  if (serialDefineWaitLines(serial, lines)) {
    while (!serialTestLines(serial, high, low))
      if (!serialAwaitLineChange(serial))
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
      if (!serialAwaitLineChange(serial))
        goto done;
    if (serialAwaitLineChange(serial)) ok = 1;
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

int
serialPrepareDevice (SerialDevice *serial) {
  if (serialReadAttributes(serial)) {
    serialCopyAttributes(&serial->originalAttributes, &serial->currentAttributes);
    serialInitializeAttributes(&serial->pendingAttributes);

    serial->linesState = 0;
    serial->waitLines = 0;

#ifdef HAVE_POSIX_THREADS
    serial->currentFlowControlProc = NULL;
    serial->pendingFlowControlProc = NULL;
    serial->flowControlRunning = 0;
#endif /* HAVE_POSIX_THREADS */

    return 1;
  }

  return 0;
}

SerialDevice *
serialOpenDevice (const char *path) {
  SerialDevice *serial;

  if ((serial = malloc(sizeof(*serial)))) {
    char *device;

    if ((device = getDevicePath(path))) {
      serial->fileDescriptor = -1;
      serial->stream = NULL;

      if (serialConnectDevice(serial, device)) {
        free(device);
        return serial;
      }

      free(device);
    }

    free(serial);
  } else {
    logMallocError();
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
  } else if (serial->fileDescriptor != -1) {
    close(serial->fileDescriptor);
  } else {
    serialDisconnectDevice(serial);
  }

  free(serial);
}

int
serialRestartDevice (SerialDevice *serial, unsigned int baud) {
  SerialLines highLines = 0;
  SerialLines lowLines = 0;
  int usingB0;

#ifdef HAVE_POSIX_THREADS
  FlowControlProc flowControlProc = serial->pendingFlowControlProc;
#endif /* HAVE_POSIX_THREADS */

  if (serial->stream) {
#if defined(GRUB_RUNTIME)
#else /* clearerr() */
    clearerr(serial->stream);
#endif /* clear error on stdio stream */
  }

  serialClearError(serial);

  if (!serialDiscardOutput(serial)) return 0;

#ifdef HAVE_POSIX_THREADS
  serial->pendingFlowControlProc = NULL;
#endif /* HAVE_POSIX_THREADS */

#ifdef B0
  if (!serialPutSpeed(&serial->pendingAttributes, B0)) return 0;
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
    if (!serialReadLines(serial, &lines)) return 0;

    {
      static const SerialLines linesTable[] = {SERIAL_LINE_DTR, SERIAL_LINE_RTS, 0};
      const SerialLines *line = linesTable;

      while (*line) {
        *((lines & *line)? &highLines: &lowLines) |= *line;
        line += 1;
      }
    }

    if (highLines)
      if (!serialWriteLines(serial, 0, highLines|lowLines))
        return 0;
  }

  approximateDelay(500);
  if (!serialDiscardInput(serial)) return 0;

  if (!usingB0)
    if (!serialWriteLines(serial, highLines, lowLines))
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
    if (!serialEnsureFileDescriptor(serial)) return NULL;

#if defined(GRUB_RUNTIME)
    errno = ENOSYS;
#else /* fdopen() */
    serial->stream = fdopen(serial->fileDescriptor, "ab+");
#endif /* create stdio stream */

    if (!serial->stream) {
      logSystemError("fdopen");
      return NULL;
    }
  }

  return serial->stream;
}

int
isSerialDevice (const char **identifier) {
#ifdef ALLOW_DOS_DEVICE_NAMES
  if (isDosDevice(*identifier, "COM")) return 1;
#endif /* ALLOW_DOS_DEVICE_NAMES */

  if (!isQualifiedDevice(identifier, "serial"))
    if (isQualifiedDevice(identifier, NULL))
      return 0;

  if (!**identifier) *identifier = SERIAL_FIRST_DEVICE;
  return 1;
}
