/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2025 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_CMDPROG
#define BRLTTY_INCLUDED_CMDPROG

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  PROG_EXIT_SUCCESS  = 0,
  PROG_EXIT_FORCE    = 1,
  PROG_EXIT_SYNTAX   = 2,
  PROG_EXIT_SEMANTIC = 3,
  PROG_EXIT_FATAL    = 4
} ProgramExitStatus;

extern const char standardStreamArgument[];
extern const char standardInputName[];
extern const char standardOutputName[];
extern const char standardErrorName[];

extern const char *getProgramName (void);
extern const char *getProgramDirectory (void);
extern char *makeProgramPath (const char *name);

extern const char *nextProgramArgument (char ***argv, int *argc, const char *description);
extern void noMoreProgramArguments (char ***argv, int *argc);

extern int toAbsoluteInstallPath (char **path);
extern char *makeHelperPath (const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMDPROG */
