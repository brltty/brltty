/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* tunetest.c - Test program for the tune playing library
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "tunes.h"
#include "misc.h"
#include "config.h"
#include "common.h"
#include "message.h"
#include "brl.h"

int refreshInterval = REFRESH_INTERVAL;
Preferences prefs;

static short
integerArgument (const char *argument, short minimum, short maximum, const char *name) {
  char *end;
  long int value = strtol(argument, &end, 0);
  if ((end == argument) || *end) {
    fprintf(stderr, "tunetest: Invalid %s: %s\n", name, argument);
  } else if (value < minimum) {
    fprintf(stderr, "tunetest: %s is less than %d: %ld\n", name, minimum, value);
  } else if (value > maximum) {
    fprintf(stderr, "tunetest: %s is greater than %d: %ld\n", name, maximum, value);
  } else {
    return value;
  }
  exit(2);
}

static unsigned int
stringArgument (const char *argument, const char *const *choices, const char *name) {
  size_t length = strlen(argument);
  const char *const *choice = choices;
  while (*choice) {
    if (strlen(*choice) >= length)
      if (strncasecmp(*choice, argument, length) == 0)
        return choice - choices;
    ++choice;
  }
  fprintf(stderr, "tunetest: Invalid %s: %s\n", name, argument);
  exit(2);
}

int
main (int argc, char *argv[]) {
  const char *const shortOptions = ":d:";
  const struct option longOptions[] = {
    {"device"   , required_argument, NULL, 'd'},
    {NULL       , 0                , NULL,  0 }
  };
  unsigned int device = tdSpeaker;

  opterr = 0;
  while (1) {
    int option = getopt_long(argc, argv, shortOptions, longOptions, NULL);
    if (option == -1) break;
    switch (option) {
      default:
        fprintf(stderr, "tunetest: Unimplemented option: -%c\n", option);
        exit(2);
      case '?':
        fprintf(stderr, "tunetest: Invalid option: -%c\n", optopt);
        exit(2);
      case ':':
        fprintf(stderr, "tunetest: Missing operand: -%c\n", optopt);
        exit(2);
      case 'd': {
        const char *devices[] = {"speaker", "dsp", "midi", "fm", NULL};
        device = stringArgument(optarg, devices, "device");
        break;
      }
    }
  }
  argv += optind; argc -= optind;
  if (!argc) {
    fprintf(stderr, "tunetest: Missing tune.\n");
  } else if (argc % 2) {
    fprintf(stderr, "tunetest: Missing duration.\n");
  } else {
    unsigned int count = argc / 2;
    ToneDefinition *tones = (ToneDefinition *)mallocWrapper((sizeof(*tones) * count) + 1);
    ToneDefinition *tone = tones;

    while (argc) {
      short note = integerArgument(*argv++, 1, 127, "note");
      short duration = integerArgument(*argv++, 1, 255, "duration");
      argc -= 2;
      *(tone++) = (ToneDefinition)TONE_NOTE(duration, note);
    }
    *tone = (ToneDefinition)TONE_STOP();

    memset(&prefs, 0, sizeof(prefs));
    prefs.sound = 1;
    setTuneDevice(device);
    {
      TuneDefinition tune = {NULL, 0, tones};
      playTune(&tune);
      closeTuneDevice(1);
    }

    free(tones);
    return 0;
  }
  return 2;
}

void
message (const unsigned char *text, short flags) {
}
void
showDotPattern (unsigned char dots, unsigned char duration) {
}
