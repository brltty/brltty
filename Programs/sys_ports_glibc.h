/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_IO_H
#include <sys/io.h>
#endif /* HAVE_SYS_IO_H */

#include "log.h"

int
enablePorts (int errorLevel, unsigned short int base, unsigned short int count) {
#ifdef HAVE_SYS_IO_H
  if (ioperm(base, count, 1) != -1) return 1;
  LogPrint(errorLevel, "Port enable error: %u.%u: %s", base, count, strerror(errno));
#else /* HAVE_SYS_IO_H */
  LogPrint(errorLevel, "I/O ports not supported.");
#endif /* HAVE_SYS_IO_H */
  return 0;
}

int
disablePorts (unsigned short int base, unsigned short int count) {
#ifdef HAVE_SYS_IO_H
  if (ioperm(base, count, 0) != -1) return 1;
  LogPrint(LOG_ERR, "Port disable error: %u.%u: %s", base, count, strerror(errno));
#endif /* HAVE_SYS_IO_H */
  return 0;
}

unsigned char
readPort1 (unsigned short int port) {
#ifdef HAVE_SYS_IO_H
  return inb(port);
#else /* HAVE_SYS_IO_H */
  return 0;
#endif /* HAVE_SYS_IO_H */
}

void
writePort1 (unsigned short int port, unsigned char value) {
#ifdef HAVE_SYS_IO_H
  outb(value, port);
#endif /* HAVE_SYS_IO_H */
}
