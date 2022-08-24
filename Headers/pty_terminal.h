/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PTY_TERMINAL
#define BRLTTY_INCLUDED_PTY_TERMINAL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const char *ptyGetTerminalType (void);

extern int ptyBeginTerminal (const char *tty);
extern void ptyEndTerminal (void);
extern void ptySynchronizeTerminal (void);

extern void ptyProcessTerminalInput (int fd);

extern int ptyParseOutputByte (unsigned char byte);
extern int ptyParseOutputBytes (const unsigned char *bytes, size_t count);

extern void ptySetTerminalLogLevel (unsigned char level);
extern void ptySetLogOutputActions (int yes);
extern void ptySetLogTerminalInput (int yes);
extern void ptySetLogTerminalOutput (int yes);
extern void ptySetLogUnexpectedSequences (int yes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PTY_TERMINAL */
