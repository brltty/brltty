/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2007 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_PROGRAM
#define BRLTTY_INCLUDED_PROGRAM

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const char *programPath;
extern const char *programName;

extern const char *packageRevision;

extern void prepareProgram (int argumentCount, char **argumentVector);
extern void makeProgramBanner (char *buffer, size_t size);

extern void fixInstallPaths (char **const *paths);
extern void fixInstallPath (char **path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PROGRAM */
