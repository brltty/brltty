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

#include "serial.h"
#include "misc.h"

typedef struct {
  int integer;
  speed_t baud;
} BaudEntry;
static const BaudEntry baudTable[] = {
#ifdef B50
  {     50, B50},
#endif /* B50 */

#ifdef B75
  {     75, B75},
#endif /* B75 */

#ifdef B110
  {    110, B110},
#endif /* B110 */

#ifdef B134
  {    134, B134},
#endif /* B134 */

#ifdef B150
  {    150, B150},
#endif /* B150 */

#ifdef B200
  {    200, B200},
#endif /* B200 */

#ifdef B300
  {    300, B300},
#endif /* B300 */

#ifdef B600
  {    600, B600},
#endif /* B600 */

#ifdef B1200
  {   1200, B1200},
#endif /* B1200 */

#ifdef B1800
  {   1800, B1800},
#endif /* B1800 */

#ifdef B2400
  {   2400, B2400},
#endif /* B2400 */

#ifdef B4800
  {   4800, B4800},
#endif /* B4800 */

#ifdef B9600
  {   9600, B9600},
#endif /* B9600 */

#ifdef B19200
  {  19200, B19200},
#endif /* B19200 */

#ifdef B38400
  {  38400, B38400},
#endif /* B38400 */

#ifdef B57600
  {  57600, B57600},
#endif /* B57600 */

#ifdef B115200
  { 115200, B115200},
#endif /* B115200 */

#ifdef B230400
  { 230400, B230400},
#endif /* B230400 */

#ifdef B460800
  { 460800, B460800},
#endif /* B460800 */

#ifdef B500000
  { 500000, B500000},
#endif /* B500000 */

#ifdef B576000
  { 576000, B576000},
#endif /* B576000 */

#ifdef B921600
  { 921600, B921600},
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

  {      0, B0}
};

static const BaudEntry *
getBaudEntry (int speed) {
  const BaudEntry *entry = baudTable;
  while (entry->integer) {
    if (speed == entry->integer) return entry;
    ++entry;
  }
  return NULL;
}

int
validateBaud (speed_t *value, const char *description, const char *word, const unsigned int *choices) {
  int speed;
  if (!*word || isInteger(&speed, word)) {
    const BaudEntry *entry = getBaudEntry(speed);
    if (entry) {
      if (choices) {
        while (*choices) {
          if (speed == *choices) break;
          ++choices;
        }

        if (!*choices) {
          LogPrint(LOG_ERR, "Unsupported %s: %d",
                   description, speed);
          return 0;
        }
      }

      *value = entry->baud;
      return 1;
    }
  }

  LogPrint(LOG_ERR, "Invalid %s: %d",
           description, speed);
  return 0;
}

int
baud2integer (speed_t baud) {
  const BaudEntry *entry = baudTable;
  while (entry->integer) {
    if (baud == entry->baud)
      return entry->integer;
    ++entry;
  }
  return -1;
}

void
initializeSerialAttributes (struct termios *attributes) {
  memset(attributes, 0, sizeof(*attributes));
  attributes->c_cflag = CREAD | CLOCAL | CS8;
  attributes->c_iflag = IGNPAR | IGNBRK;
}

int
setSerialSpeed (struct termios *attributes, int speed) {
  const BaudEntry *entry = getBaudEntry(speed);
  if (entry) {
    if (cfsetispeed(attributes, entry->baud) != -1) {
      if (cfsetospeed(attributes, entry->baud) != -1) {
        return 1;
      } else {
        LogError("cfsetospeed");
      }
    } else {
      LogError("cfsetispeed");
    }
  } else {
    LogPrint(LOG_WARNING, "Unknown serial speed: %d", speed);
  }
  return 0;
}

int
setSerialDataBits (struct termios *attributes, int bits) {
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
      LogPrint(LOG_WARNING, "Unknown serial data bit count: %d", bits);
      return 0;
  }

  attributes->c_cflag &= ~CSIZE;
  attributes->c_cflag |= size;
  return 1;
}

int
setSerialStopBits (struct termios *attributes, int bits) {
  if (bits == 1) {
    attributes->c_cflag &= ~CSTOPB;
  } else if (bits == 2) {
    attributes->c_cflag |= CSTOPB;
  } else {
    LogPrint(LOG_WARNING, "Unknown serial stop bit count: %d", bits);
    return 0;
  }
  return 1;
}

int
setSerialParity (struct termios *attributes, SerialParity parity) {
  if (parity == SERIAL_PARITY_NONE) {
    attributes->c_cflag &= ~(PARENB | PARODD);
  } else {
    if (parity == SERIAL_PARITY_EVEN) {
      attributes->c_cflag &= ~PARODD;
    } else if (parity == SERIAL_PARITY_ODD) {
      attributes->c_cflag |= PARODD;
    } else {
      LogPrint(LOG_WARNING, "Unknown serial parity: %c", parity);
      return 0;
    }
    attributes->c_cflag |= PARENB;
  }
  return 1;
}

int
setSerialFlowControl (struct termios *attributes, SerialFlowControl flow) {
#ifdef CRTSCTS
  if (flow & SERIAL_FLOW_HARDWARE) {
    attributes->c_cflag |= CRTSCTS;
    flow &= ~SERIAL_FLOW_HARDWARE;
  } else {
    attributes->c_cflag &= ~CRTSCTS;
  }
#else /* CRTSCTS */
#warning hardware flow control not settable on this platform
#endif /* CRTSCTS */

#ifdef IXOFF
  if (flow & SERIAL_FLOW_SOFTWARE_INPUT) {
    attributes->c_iflag |= IXOFF;
    flow &= ~SERIAL_FLOW_SOFTWARE_INPUT;
  } else {
    attributes->c_iflag &= ~IXOFF;
  }
#else /* IXOFF */
#warning software input flow control not settable on this platform
#endif /* IXOFF */

#ifdef IXON
  if (flow & SERIAL_FLOW_SOFTWARE_OUTPUT) {
    attributes->c_iflag |= IXON;
    flow &= ~SERIAL_FLOW_SOFTWARE_OUTPUT;
  } else {
    attributes->c_iflag &= ~IXON;
  }
#else /* IXON */
#warning software output flow control not settable on this platform
#endif /* IXON */

  if (flow) {
    LogPrint(LOG_WARNING, "Unknown serial flow control: 0X%02X", flow);
    return 0;
  }

  return 1;
}

int
applySerialAttributes (const struct termios *attributes, int device) {
  if (tcsetattr(device, TCSANOW, attributes) != -1) return 1;
  LogError("tcsetattr");
  return 0;
}

int
isSerialDevice (const char **path) {
  if (isQualifiedDevice(path, "serial")) return 1;
  return !isQualifiedDevice(path, NULL);
}

int
openSerialDevice (const char *path, int *descriptor, struct termios *attributes) {
  char *device;
  if ((device = getDevicePath(path))) {
    if ((*descriptor = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK)) != -1) {
      if (isatty(*descriptor)) {
        if (setBlockingIo(*descriptor, 1)) {
          if (!attributes || (tcgetattr(*descriptor, attributes) != -1)) {
            LogPrint(LOG_DEBUG, "Serial device opened: %s: fd=%d", device, *descriptor);
            free(device);
            return 1;
          } else {
            LogPrint(LOG_ERR, "Cannot get attributes for '%s': %s", device, strerror(errno));
          }
        }
      } else {
        LogPrint(LOG_ERR, "Not a serial device: %s", device);
      }
      close(*descriptor);
      *descriptor = -1;
    } else {
      LogPrint(LOG_ERR, "Cannot open '%s': %s", device, strerror(errno));
    }
    free(device);
  }
  return 0;
}

void
rawSerialDevice (struct termios *attributes) {
  attributes->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  attributes->c_oflag &= ~OPOST;
  attributes->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  attributes->c_cflag &= ~(CSIZE | PARENB);
  attributes->c_cflag |= CS8;
}

int
flushSerialInput (int descriptor) {
  if (tcflush(descriptor, TCIFLUSH) != -1) return 1;
  LogError("TCIFLUSH");
  return 0;
}

int
flushSerialOutput (int descriptor) {
  if (tcflush(descriptor, TCOFLUSH) != -1) return 1;
  LogError("TCOFLUSH");
  return 0;
}

int
setSerialDevice (int descriptor, struct termios *attributes, speed_t baud) {
  if (cfsetispeed(attributes, baud) != -1) {
    if (cfsetospeed(attributes, baud) != -1) {
      if (applySerialAttributes(attributes, descriptor)) {
        return 1;
      } else {
        LogError("Serial device attributes set");
      }
    } else {
      LogError("Serial device output speed set");
    }
  } else {
    LogError("Serial device input speed set");
  }
  return 0;
}

int
resetSerialDevice (int descriptor, struct termios *attributes, speed_t baud) {
  if (flushSerialOutput(descriptor)) {
    if (setSerialDevice(descriptor, attributes, B0)) {
      delay(500);
      if (flushSerialInput(descriptor)) {
        if (setSerialDevice(descriptor, attributes, baud)) {
          return 1;
        }
      }
    }
  }
  return 0;
}
