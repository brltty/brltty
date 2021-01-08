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

#ifndef BRLTTY_INCLUDED_SCR_HURD
#define BRLTTY_INCLUDED_SCR_HURD

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HURD_CONSDIR		"/dev/cons"
#define HURD_VCSDIR		"/dev/vcs"
#define HURD_INPUTPATH		HURD_VCSDIR "/%u/input"
#define HURD_DISPLAYPATH	HURD_VCSDIR "/%u/display"
#define HURD_CURVCSPATH		HURD_CONSDIR "/vcs"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SCR_HURD */
