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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  CFG_OK,              /* No error. */
  CFG_NoValue,         /* Operand not specified. */
  CFG_BadValue,        /* Bad operand specified. */
  CFG_TooMany,         /* Too many operands. */
  CFG_Duplicate        /* Directive specified more than once. */
} ConfigurationLineStatus;

#define OPT_Hidden 0X1

typedef struct {
   char letter;
   const char *word;
   const char *argument;
   ConfigurationLineStatus (*configure) (const char *delimiters); 
   int flags;
   const char *description;
} OptionEntry;

#define BEGIN_OPTION_TABLE static const OptionEntry optionTable[] = {
#define END_OPTION_TABLE \
  {'h', "help", NULL, NULL, 0, \
   "Print this usage summary and exit."}, \
  {'H', "full-help", NULL, NULL, OPT_Hidden, \
   "Print this full usage summary and exit."} \
}; static unsigned int optionCount = sizeof(optionTable) / sizeof(optionTable[0]);

typedef int (*OptionHandler) (const int option);
extern int processOptions (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  OptionHandler handleOption,
  int *argc,
  char ***argv,
  const char *argumentsSummary
);

extern int processConfigurationFile (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  const char *path,
  int optional
);

extern const char *programPath;
extern const char *programName;

extern short integerArgument (
  const char *argument,
  short minimum,
  short maximum,
  const char *name
);
extern unsigned int wordArgument (
  const char *argument,
  const char *const *choices,
  const char *name
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _OPTIONS_H */
