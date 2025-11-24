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

#ifndef BRLTTY_INCLUDED_CMDLINE_TYPES
#define BRLTTY_INCLUDED_CMDLINE_TYPES

#include "strfmt_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define OPT_WORD_TRUE "on"
#define OPT_WORD_FALSE "off"

typedef enum {
  OPT_Extend	= 0X01,
  OPT_Config	= 0X02,
  OPT_EnvVar	= 0X04,
  OPT_Format  	= 0X08,
} CommandLineOptionFlag;

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
} CommandLineOption;

typedef struct {
  const CommandLineOption *table;
  size_t count;
} CommandLineOptions;

#define BEGIN_COMMAND_LINE_OPTIONS(name) \
static const CommandLineOption name##Table[] = { \
  { .word = "help", \
    .letter = 'h', \
    .description = strtext("Show this usage summary, and then exit.") \
  },

#define END_COMMAND_LINE_OPTIONS(name) }; \
static const CommandLineOptions name = { \
  .table = name##Table, \
  .count = ARRAY_COUNT(name##Table), \
};

typedef struct {
  const char *name;
  const char *description;
  const char **setting;
  char optional;
} CommandLineParameter;

typedef struct {
  const CommandLineParameter *table;
  size_t count;
} CommandLineParameters;

#define BEGIN_COMMAND_LINE_PARAMETERS(name) \
static const CommandLineParameter name##Table[] = {

#define END_COMMAND_LINE_PARAMETERS(name) }; \
static const CommandLineParameters name = { \
  .table = name##Table, \
  .count = ARRAY_COUNT(name##Table), \
};

#define DECLARE_COMMAND_LINE_NOTES(name) const char *const name[]
#define START_COMMAND_LINE_NOTES(name) DECLARE_COMMAND_LINE_NOTES(name) = {
#define BEGIN_COMMAND_LINE_NOTES(name) static START_COMMAND_LINE_NOTES(name)
#define END_COMMAND_LINE_NOTES NULL};
#define COMMAND_LINE_NOTES(...) (const char *const *const []){__VA_ARGS__, NULL}

typedef struct {
  const char *name;
  const char *purpose;
  const char *oldParameters;

  const CommandLineOptions *options;
  const CommandLineParameters *parameters;
  const char *const *const *notes;

  struct {
    const char *name;
    const char *description;
  } extraParameters;

  char **configurationFile;
  int *doEnvironmentVariables;
  int *doBootParameters;
} CommandLineDescriptor;

#define BEGIN_COMMAND_LINE_DESCRIPTOR(name) \
static const CommandLineDescriptor name = {

#define END_COMMAND_LINE_DESCRIPTOR };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_CMDLINE_TYPES */
