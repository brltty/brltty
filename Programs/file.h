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

#ifndef BRLTTY_INCLUDED_FILE
#define BRLTTY_INCLUDED_FILE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdarg.h>

extern int isPathDelimiter (const char character);
extern int isAbsolutePath (const char *path);
extern char *getPathDirectory (const char *path);
extern const char *locatePathName (const char *path);
extern const char *locatePathExtension (const char *path);
extern int isExplicitPath (const char *path);

extern char *makePath (const char *directory, const char *file);
extern char *ensureFileExtension (const char *path, const char *extension);
extern char *makeFilePath (const char *directory, const char *name, const char *extension);

extern int testPath (const char *path);
extern int testFilePath (const char *path);
extern int testProgramPath (const char *path);
extern int testDirectoryPath (const char *path);

extern int createDirectory (const char *path);
extern int ensureDirectory (const char *path);

extern const char *writableDirectory;
extern const char *getWritableDirectory (void);
extern char *makeWritablePath (const char *file);

extern char *getWorkingDirectory (void);
extern int setWorkingDirectory (const char *path);

extern char *getHomeDirectory (void);
extern const char *getOverrideDirectory (void);

extern FILE *openFile (const char *path, const char *mode, int optional);
extern FILE *openDataFile (const char *path, const char *mode, int optional);

extern int acquireFileLock (int file, int exclusive);
extern int attemptFileLock (int file, int exclusive);
extern int releaseFileLock (int file);

typedef int LineHandler (char *line, void *data);
extern int processLines (FILE *file, LineHandler handleLine, void *data);
extern int readLine (FILE *file, char **buffer, size_t *size);

extern size_t formatInputError (char *buffer, size_t size, const char *file, const int *line, const char *format, va_list argp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_FILE */
