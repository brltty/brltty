/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2023 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_TR_BRLDEFS
#define BRLTTY_INCLUDED_TR_BRLDEFS

#define TR_MAX_DATA_LENGTH 0XFF
#define TR_MAX_PACKET_SIZE (3 + TR_MAX_DATA_LENGTH + 2)

#define TR_PKT_START_OF_MESSAGE 0X80
#define TR_PKT_END_OF_MESSAGE   0X81

#define TR_CMD_ACTUATE 0X02

#endif /* BRLTTY_INCLUDED_TR_BRLDEFS */ 
