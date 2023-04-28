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

/* ExternalSpeech/speech.h */

/* The following are the default parameters that will be used if no parameter
   is specified on the command-line (-p) or in the brltty.conf file
   (speech-driverparm option). */

/* The default path of the UNIX-domain socket for the external helper program. */
#define XS_DEFAULT_SOCKET_PATH "/tmp/exs-data"

/* The maxdimum amount of time that a write to the socket may take. */
#define XS_WRITE_TIMEOUT 2000
