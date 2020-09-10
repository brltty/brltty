/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_IHEX
#define BRLTTY_INCLUDED_IHEX

#include "ihex_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IHEX_TYPE_DATA 0X00
#define IHEX_TYPE_END  0X01

extern size_t ihexRecordLength (size_t count);

extern int ihexMakeRecord (
  char *buffer, size_t size,
  uint8_t type, uint16_t address,
  const unsigned char *data, uint8_t count
);

extern int ihexMakeDataRecord (
  char *buffer, size_t size,
  uint16_t address, const unsigned char *data, uint8_t count
);

extern int ihexMakeEndRecord (char *buffer, size_t size);

extern int ihexProcessFile (
  const char *path, IhexRecordHandler *handler, void *data
);

#define IHEX_FILES_SUBDIRECTORY "Firmware"
#define IHEX_FILE_EXTENSION ".ihex"

extern char *ihexEnsureExtension (const char *path);
extern char *ihexMakePath (const char *directory, const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IHEX */
