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

#ifndef _SCR_HURD_H
#define _SCR_HURD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HURD_CONSDIR		"/dev/cons"
#define HURD_VCSDIR		"/dev/vcs"
#define HURD_INPUTPATH		HURD_VCSDIR "/input"
#define HURD_DISPLAYPATH	HURD_VCSDIR "/display"
#define HURD_CURVCSPATH		HURD_CONSDIR "/vcs"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SCR_HURD_H */
