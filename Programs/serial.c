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

#include "prologue.h"

#include "serial_internal.h"

const BaudEntry *
getBaudEntry (unsigned int baud) {
  const BaudEntry *entry = serialBaudTable;
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
}

int
serialSetBaud (SerialDevice *serial, unsigned int baud) {
  const BaudEntry *entry = getBaudEntry(baud);

  if (entry) {
    if (serialPutSpeed(serial, entry->speed)) {
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
    const BaudEntry *entry = getBaudEntry(*baud);

    if (entry) {
      if (!choices) return 1;

      while (*choices) {
        if (*baud == *choices) return 1;
        choices += 1;
      }

      logMessage(LOG_ERR, "unsupported %s: %u", description, *baud);
      return 0;
    }
  }

  logMessage(LOG_ERR, "invalid %s: %u", description, *baud);
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

  while (1) {
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
    pthread_cancel(serial->flowControlThread);
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

  serialPutModemState(&serial->pendingAttributes, !serial->pendingFlowControlProc);
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
  unsigned int size = 1 + parameters->dataBits;
  size += (parameters->stopBits == SERIAL_STOP_1)? 1: 2;
  if (parameters->parity != SERIAL_PARITY_NONE) size += 1;
  return size;
}

unsigned int
serialGetCharacterBits (SerialDevice *serial) {
  const SerialAttributes *attributes = &serial->pendingAttributes;
  return 1
       + serialGetDataBits(attributes)
       + serialGetParityBits(attributes)
       + serialGetStopBits(attributes)
       ;
}

int
serialDiscardInput (SerialDevice *serial) {
#if defined(__MINGW32__)
  if (PurgeComm(serial->fileHandle, PURGE_RXCLEAR)) return 1;
  logWindowsSystemError("PurgeComm");
#elif defined(__MSDOS__)
  logMessage(LOG_DEBUG, "function not supported: %s", __func__);
  return 1;
#else /* UNIX */
  if (tcflush(serial->fileDescriptor, TCIFLUSH) != -1) return 1;
  if (errno == EINVAL) return 1;
  logSystemError("TCIFLUSH");
#endif /* discard input */
  return 0;
}

int
serialDiscardOutput (SerialDevice *serial) {
#if defined(__MINGW32__)
  if (PurgeComm(serial->fileHandle, PURGE_TXCLEAR)) return 1;
  logWindowsSystemError("PurgeComm");
#elif defined(__MSDOS__)
  logMessage(LOG_DEBUG, "function not supported: %s", __func__);
  return 1;
#else /* UNIX */
  if (tcflush(serial->fileDescriptor, TCOFLUSH) != -1) return 1;
  if (errno == EINVAL) return 1;
  logSystemError("TCOFLUSH");
#endif /* discard output */
  return 0;
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
serialDrainOutput (SerialDevice *serial) {
  if (!serialFlushOutput(serial)) return 0;

#if defined(__MINGW32__)
  if (FlushFileBuffers(serial->fileHandle)) return 1;
  logWindowsSystemError("FlushFileBuffers");
#elif defined(__MSDOS__)
  logMessage(LOG_DEBUG, "function not supported: %s", __func__);
  return 1;
#else /* UNIX */
  do {
    if (tcdrain(serial->fileDescriptor) != -1) return 1;
  } while (errno == EINTR);
  logSystemError("tcdrain");
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
  logWindowsSystemError("GetCommState");
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
  logSystemError("tcgetattr");
#endif /* read attributes */
  return 0;
}

static int
serialWriteAttributes (SerialDevice *serial, const SerialAttributes *attributes) {
  if (!serialCompareAttributes(attributes, &serial->currentAttributes)) {
    if (!serialDrainOutput(serial)) return 0;

#if defined(__MINGW32__)
    if (!SetCommState(serial->fileHandle, (SerialAttributes *)attributes)) {
      logWindowsSystemError("SetCommState");
      return 0;
    }
#elif defined(__MSDOS__)
    {
      if (attributes->speed.biosBPS <= 7) {
        if (bioscom(0, attributes->bios.byte, serial->port) & 0X0700) {
          logMessage(LOG_ERR, "serialWriteAttributes failed");
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
      logSystemError("tcsetattr");
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
      logWindowsSystemError("SetCommTimeouts serialAwaitInput");
      setSystemErrno();
      return 0;
    }

    if (!ReadFile(serial->fileHandle, &c, 1, &bytesRead, NULL)) {
      logWindowsSystemError("ReadFile");
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
      logWindowsSystemError("SetCommTimeouts serialReadChunk1");
      setSystemErrno();
      return 0;
    }

    if (!ReadFile(serial->fileHandle, buffer+*offset, count, &bytesRead, NULL)) {
      logWindowsSystemError("ReadFile");
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
    logWindowsSystemError("SetCommTimeouts serialReadChunk2");
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
  logWindowsSystemError("ReadFile");
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
      logWindowsSystemError("SetCommTimeouts serialWriteData");
      setSystemErrno();
      return -1;
    }

    while (left && WriteFile(serial->fileHandle, data, left, &bytesWritten, NULL)) {
      if (!bytesWritten) break;
      left -= bytesWritten;
      data += bytesWritten;
    }

    if (!left) return size;
    logWindowsSystemError("WriteFile");
#else /* __MINGW32__ */
    if (writeData(serial->fileDescriptor, data, size) != -1) return size;
    logSystemError("serial write");
#endif /* __MINGW32__ */
  }
  return -1;
}

static int
serialGetLines (SerialDevice *serial, SerialLines *lines) {
#if defined(__MINGW32__)
  DCB dcb;
  if (!GetCommModemStatus(serial->fileHandle, &serial->linesState)) {
    logWindowsSystemError("GetCommModemStatus");
    return 0;
  }
  dcb.DCBlength = sizeof(dcb);
  if (!GetCommState(serial->fileHandle, &dcb)) {
    logWindowsSystemError("GetCommState");
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
    logSystemError("TIOCMGET");
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
    logWindowsSystemError("SetCommState");
  } else {
    logWindowsSystemError("GetCommState");
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
    logSystemError("TIOCMSET");
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
      logWindowsSystemError("SetCommMask");
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
  logWindowsSystemError("WaitCommEvent");
#elif defined(TIOCMIWAIT)
  if (ioctl(serial->fileDescriptor, TIOCMIWAIT, serial->waitLines) != -1) return 1;
  logSystemError("TIOCMIWAIT");
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

            logMessage(LOG_DEBUG, "serial device opened: %s: fd=%d",
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
          logMessage(LOG_ERR, "could not determine serial device port number: %s", device);
        }

        if (truePath) free(truePath);
#else /* __MSDOS__ */
        } else {
          logMessage(LOG_ERR, "not a serial device: %s", device);
        }
#endif /* __MSDOS__ */

        close(serial->fileDescriptor);
#endif /* __MINGW32__ */
      } else {
#ifdef __MINGW32__
        logWindowsSystemError("CreateFile");
        logMessage(LOG_ERR, "cannot open serial device: %s", device);
#else /* __MINGW32__ */
        logMessage(LOG_ERR, "cannot open serial device: %s: %s", device, strerror(errno));
#endif /* __MINGW32__ */
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
serialRestartDevice (SerialDevice *serial, unsigned int baud) {
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
  if (!serialPutSpeed(serial, B0)) return 0;
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
        logSystemError("cygwin_attach_handle_to_fd");
#else /* __CYGWIN32__ */
      if ((serial->fileDescriptor = _open_osfhandle((long)serial->fileHandle, O_RDWR)) < 0) {
        logSystemError("open_osfhandle");
#endif /* __CYGWIN32__ */
        return NULL;
      }
    }
#endif /* __MINGW32__ */

    if (!(serial->stream = fdopen(serial->fileDescriptor, "ab+"))) {
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

  if (!**identifier) *identifier = FIRST_SERIAL_DEVICE;
  return 1;
}
