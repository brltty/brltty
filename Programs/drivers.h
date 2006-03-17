/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_DRIVERS
#define BRLTTY_INCLUDED_DRIVERS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  const void *address;
  const char *const *code;
} DriverEntry;

extern int isDriverAvailable (const char *code, const char *codes);
extern int isDriverIncluded (const char *code, const DriverEntry *table);
extern int haveDriver (const char *code, const char *codes, const DriverEntry *table);
extern const char *getDefaultDriver (const DriverEntry *table);

extern const void *loadDriver (
  const char *driverCode, void **driverObject,
  const char *driverDirectory, const DriverEntry *driverTable,
  const char *driverType, char driverCharacter, const char *driverSymbol,
  const void *nullAddress, const char *nullCode
);

extern void identifyDriver (
  const char *type,
  const char *name, const char *version,
  const char *date, const char *time,
  const char *copyright,
  int full
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_DRIVERS */
