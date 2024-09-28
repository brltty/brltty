/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2024 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PROFILE
#define BRLTTY_INCLUDED_PROFILE

#include "profile_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *makeProfilePath (const ProfileDescriptor *profile, const char *directory, const char *name);
extern int activateProfile (const ProfileDescriptor *profile, const char *directory, const char *name);
extern int deactivateProfile (const ProfileDescriptor *profile);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PROFILE */
