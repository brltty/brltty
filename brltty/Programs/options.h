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

#define OPT_Hidden	0X01
#define OPT_Extend	0X02
#define OPT_Config	0X04
#define OPT_Environ	0X08

typedef struct {
  const char *word;
  const char *argument;
  unsigned char letter;
  unsigned char bootParameter;
  unsigned char flags;

  void *setting;
  const char *defaultSetting;

  const char *description;
} OptionEntry;

#define BEGIN_OPTION_TABLE static const OptionEntry optionTable[] = {
#define END_OPTION_TABLE \
  {"help", NULL, 'h', 0, 0, \
   NULL, NULL, \
   "Print a usage summary and exit."}, \
\
  {"full-help", NULL, 'H', 0, OPT_Hidden, \
   NULL, NULL, \
   "Print a full usage summary and exit."} \
}; \
static unsigned int optionCount = sizeof(optionTable) / sizeof(optionTable[0]);

extern int processOptions (
  const OptionEntry *optionTable,
  unsigned int optionCount,
  const char *applicationName,
  int *argumentCount,
  char ***argumentVector,
  int *doBootParameters,
  int *doEnvironmentVariables,
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
