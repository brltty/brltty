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

#ifndef BRLTTY_INCLUDED_CMDLINE_TYPES
#define BRLTTY_INCLUDED_CMDLINE_TYPES

#include "strfmth.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FLAG_TRUE_WORD "on"
#define FLAG_FALSE_WORD "off"

typedef enum {
  OPT_Hidden	= 0X01,
  OPT_Extend	= 0X02,
  OPT_Config	= 0X04,
  OPT_EnvVar	= 0X08,
  OPT_Format  	= 0X10
} OptionFlag;

typedef struct {
  const char *word;
  const char *argument;
  const char *description;

  struct {
    const char *setting;
    int (*adjust) (char **setting);
  } internal;

  unsigned char letter;
  unsigned char bootParameter;
  unsigned char flags;

  union {
    int *flag;
    char **string;
  } setting;

  union {
    const char *const *array;
    STR_DECLARE_FORMATTER((*format), unsigned int index);
  } strings;
} OptionEntry;

typedef struct {
  const OptionEntry *table;
  size_t count;
} OptionsDescriptor;

#define BEGIN_OPTION_TABLE(name) \
static const OptionEntry name[] = { \
  { .word = "help", \
    .letter = 'h', \
    .description = strtext("Show a usage summary that only contains commonly used options, and then exit.") \
  }, \
  { .word = "full-help", \
    .letter = 'H', \
    .description = strtext("Show a usage summary that contains all of the options, and then exit.") \
  },

#define END_OPTION_TABLE(name) }; \
  static const OptionsDescriptor name##Descriptor = { \
    .table = name, \
    .count = ARRAY_COUNT(name), \
  };

#define DECLARE_USAGE_NOTES(name) const char *const name[]
#define BEGIN_USAGE_NOTES(name) DECLARE_USAGE_NOTES(name) = {
#define END_USAGE_NOTES NULL};
#define USAGE_NOTES(...) (const char *const *const []){__VA_ARGS__, NULL}

typedef struct {
  const char *purpose;
  const char *parameters;
  const char *const *const *notes;
} UsageDescriptor;

typedef struct {
  const OptionsDescriptor *options;

  int *doBootParameters;
  int *doEnvironmentVariables;
  char **configurationFile;

  const char *applicationName;
  const UsageDescriptor usage;
} CommandLineDescriptor;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMDLINE_TYPES */
