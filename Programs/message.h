/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
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

#ifndef _MESSAGE_H
#define _MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* message.h - send a message to Braille and speech */

/* Prototype: */
extern void message (const char *, short);


/* Flags for the second argument: */
#define MSG_SILENT 1		/* Prevent output to speech */
#define MSG_WAITKEY 2		/* Wait for a key after the message is displayed */
#define MSG_NODELAY 4 /* message now automatically delays for DISPDEL ms,
			 unless this flag is set. */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MESSAGE_H */
