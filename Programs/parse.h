/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_PARSE
#define BRLTTY_INCLUDED_PARSE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char **splitString (const char *string, char delimiter, int *count);
extern void deallocateStrings (char **array);
extern char *joinStrings (const char *const *strings, int count);

extern int rescaleInteger (int value, int from, int to);

extern int isInteger (int *value, const char *string);
extern int isUnsignedInteger (unsigned int *value, const char *string);
extern int isLogLevel (int *level, const char *string);

extern int validateInteger (int *value, const char *string, const int *minimum, const int *maximum);
extern int validateChoice (unsigned int *value, const char *string, const char *const *choices);
extern int validateFlag (unsigned int *value, const char *string, const char *on, const char *off);
extern int validateOnOff (unsigned int *value, const char *string);
extern int validateYesNo (unsigned int *value, const char *string);

#ifndef NO_FLOAT
extern int isFloat (float *value, const char *string);
extern int validateFloat (float *value, const char *string, const float *minimum, const float *maximum);
#endif /* NO_FLOAT */

extern char **getParameters (const char *const *names, const char *qualifier, const char *parameters);
extern void logParameters (const char *const *names, char **values, const char *description);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PARSE */
