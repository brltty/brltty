/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

#ifndef _SYSMISC_H
#define _SYSMISC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  const void *address;
  const char *const *identifier;
} DriverEntry;

extern const void *
loadDriver (
  const char *driver,
  const char *driverDirectory, const char *driverSymbol,
  const char *driverType, char driverCharacter,
  const DriverEntry *driverTable,
  const void *nullAddress, const char *nullIdentifier
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSMISC_H */
