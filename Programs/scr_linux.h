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

#ifndef _SCR_LINUX_H
#define _SCR_LINUX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LINUX_SCREEN_DEVICES  "/dev/vcsa /dev/vcsa0 /dev/vcc/a"
#define LINUX_CONSOLE_DEVICES "/dev/tty0 /dev/vc/0"

typedef unsigned short int UnicodeNumber;
typedef UnicodeNumber ApplicationCharacterMap[0X100];

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SCR_LINUX_H */
