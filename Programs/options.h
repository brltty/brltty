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

#ifndef BRLTTY_INCLUDED_OPTIONS
#define BRLTTY_INCLUDED_OPTIONS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define OPT_Hidden	0X01
#define OPT_Extend	0X02
#define OPT_Config	0X04
#define OPT_Environ	0X08

#define FLAG_TRUE_WORD "on"
#define FLAG_FALSE_WORD "off"

typedef struct {
  const char *word;
  const char *argument;
  unsigned char letter;
  unsigned char bootParameter;
  unsigned char flags;

  void *setting;
  const char *defaultSetting;

  const char *description;
  const char *const *strings;
} OptionEntry;

#define BEGIN_OPTION_TABLE static const OptionEntry optionTable[] = {
#define END_OPTION_TABLE \
  {"help", NULL, 'h', 0, 0, \
   NULL, NULL, \
   strtext("Print a usage summary and exit.")}, \
\
  {"full-help", NULL, 'H', 0, OPT_Hidden, \
   NULL, NULL, \
   strtext("Print a full usage summary and exit.")} \
}; \
static unsigned int optionCount = ARRAY_COUNT(optionTable);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_OPTIONS */
