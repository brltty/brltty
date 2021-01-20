/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PGMLOCALE
#define BRLTTY_INCLUDED_PGMLOCALE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void setProgramLocale (void);

extern const char *getLocaleString (void);
extern const char *getLocaleDomain (void);
extern const char *getLocaleDirectory (void);

extern const char *setLocaleDomain (const char *domain);
extern const char *setLocaleDirectory (const char *directory);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PGMLOCALE */
