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

#define OPT_Hidden	0X1
#define OPT_Extend	0X2
#define OPT_Config	0X4

typedef struct {
  unsigned char letter;
  const char *word;
  const char *argument;
  int flags;

  char **setting;
  const char *defaultSetting;
  const char *environmentVariable;
  int bootParameter;

  const char *description;
} OptionEntry;

#define BEGIN_OPTION_TABLE static const OptionEntry optionTable[] = {
#define END_OPTION_TABLE \
  {'h', "help", NULL, 0, \
   NULL, NULL, NULL, -1, \
   "Print this usage summary and exit."}, \
\
  {'H', "full-help", NULL, OPT_Hidden, \
   NULL, NULL, NULL, -1, \
   "Print this full usage summary and exit."} \
}; \
static unsigned int optionCount = sizeof(optionTable) / sizeof(optionTable[0]);

typedef int (*OptionHandler) (const int option);
extern int processOptions (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  OptionHandler handleOption,
  int *argc,
  char ***argv,
  const char *bootParameter,
  int *environmentVariables,
  char **configurationFile,
  const char *argumentsSummary
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
