/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#include "misc.h"
#include "bluez.h"
#include "bluez_internal.h"

int
isBluetoothDevice (const char **path) {
  return isQualifiedDevice(path, "bluez:");
}

int
parseBluetoothAddress (bdaddr_t *address, const char *string) {
  const char *start = string;
  int index = sizeof(address->b);
  while (--index >= 0) {
    char *end;
    long int value = strtol(start, &end, 16);
    if (end == start) return 0;
    if (value < 0) return 0;
    if (value > 0XFF) return 0;
    address->b[index] = value;
    if (!*end) break;
    if (*end != ':') return 0;
    start = end + 1;
  }
  if (index < 0) return 0;
  while (--index >= 0) address->b[index] = 0;
  return 1;
}
