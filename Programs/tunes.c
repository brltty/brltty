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

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "system.h"
#include "message.h"
#include "brl.h"
#include "brltty.h"
#include "prefs.h"
#include "tunes.h"
#include "notes.h"

static const TuneElement elements_braille_on[] = {
  TUNE_NOTE( 60,  64),
  TUNE_NOTE(100,  69),
  TUNE_STOP()
};
const TuneDefinition tune_braille_on = {
  NULL, 0, elements_braille_on
};

static const TuneElement elements_braille_off[] = {
  TUNE_NOTE( 60,  64),
  TUNE_NOTE( 60,  57),
  TUNE_STOP()
};
const TuneDefinition tune_braille_off = {
  NULL, 0, elements_braille_off
};

static const TuneElement elements_command_done[] = {
  TUNE_NOTE( 40,  74),
  TUNE_REST( 30),
  TUNE_NOTE( 40,  74),
  TUNE_REST( 40),
  TUNE_NOTE(140,  74),
  TUNE_REST( 20),
  TUNE_NOTE( 50,  79),
  TUNE_STOP()
};
const TuneDefinition tune_command_done = {
  strtext("Done"), 0, elements_command_done
};

static const TuneElement elements_command_rejected[] = {
  TUNE_NOTE(100,  78),
  TUNE_STOP()
};
const TuneDefinition tune_command_rejected = {
  NULL, TUNE_TACTILE(50,BRL_DOT1|BRL_DOT2|BRL_DOT3|BRL_DOT4|BRL_DOT5|BRL_DOT6|BRL_DOT7|BRL_DOT8), elements_command_rejected
};

static const TuneElement elements_mark_set[] = {
  TUNE_NOTE( 20,  83),
  TUNE_NOTE( 15,  81),
  TUNE_NOTE( 15,  79),
  TUNE_NOTE( 25,  84),
  TUNE_STOP()
};
const TuneDefinition tune_mark_set = {
  NULL, 0, elements_mark_set
};

static const TuneElement elements_clipboard_begin[] = {
  TUNE_NOTE( 40,  74),
  TUNE_NOTE( 20,  86),
  TUNE_STOP()
};
const TuneDefinition tune_clipboard_begin = {
  NULL, 0, elements_clipboard_begin
};

static const TuneElement elements_clipboard_end[] = {
  TUNE_NOTE( 50,  86),
  TUNE_NOTE( 30,  74),
  TUNE_STOP()
};
const TuneDefinition tune_clipboard_end = {
  NULL, 0, elements_clipboard_end
};

static const TuneElement elements_toggle_on[] = {
  TUNE_NOTE( 30,  74),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  79),
  TUNE_REST( 30),
  TUNE_NOTE( 40,  86),
  TUNE_STOP()
};
const TuneDefinition tune_toggle_on = {
  NULL, TUNE_TACTILE(30,BRL_DOT1|BRL_DOT2|BRL_DOT4|BRL_DOT5), elements_toggle_on
};

static const TuneElement elements_toggle_off[] = {
  TUNE_NOTE( 30,  86),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  79),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  74),
  TUNE_STOP()
};
const TuneDefinition tune_toggle_off = {
  NULL, TUNE_TACTILE(30,BRL_DOT3|BRL_DOT7|BRL_DOT6|BRL_DOT8), elements_toggle_off
};

static const TuneElement elements_cursor_linked[] = {
  TUNE_NOTE(  7,  80),
  TUNE_NOTE(  7,  79),
  TUNE_NOTE( 12,  76),
  TUNE_STOP()
};
const TuneDefinition tune_cursor_linked = {
  NULL, 0, elements_cursor_linked
};

static const TuneElement elements_cursor_unlinked[] = {
  TUNE_NOTE(  7,  78),
  TUNE_NOTE(  7,  79),
  TUNE_NOTE( 20,  83),
  TUNE_STOP()
};
const TuneDefinition tune_cursor_unlinked = {
  NULL, 0, elements_cursor_unlinked
};

static const TuneElement elements_screen_frozen[] = {
  TUNE_NOTE(  5,  58),
  TUNE_NOTE(  5,  59),
  TUNE_NOTE(  5,  60),
  TUNE_NOTE(  5,  61),
  TUNE_NOTE(  5,  62),
  TUNE_NOTE(  5,  63),
  TUNE_NOTE(  5,  64),
  TUNE_NOTE(  5,  65),
  TUNE_NOTE(  5,  66),
  TUNE_NOTE(  5,  67),
  TUNE_NOTE(  5,  68),
  TUNE_NOTE(  5,  69),
  TUNE_NOTE(  5,  70),
  TUNE_NOTE(  5,  71),
  TUNE_NOTE(  5,  72),
  TUNE_NOTE(  5,  73),
  TUNE_NOTE(  5,  74),
  TUNE_NOTE(  5,  76),
  TUNE_NOTE(  5,  78),
  TUNE_NOTE(  5,  80),
  TUNE_NOTE(  5,  83),
  TUNE_NOTE(  5,  86),
  TUNE_NOTE(  5,  90),
  TUNE_NOTE(  5,  95),
  TUNE_STOP()
};
const TuneDefinition tune_screen_frozen = {
  strtext("Frozen"), 0, elements_screen_frozen
};

static const TuneElement elements_screen_unfrozen[] = {
  TUNE_NOTE(  5,  95),
  TUNE_NOTE(  5,  90),
  TUNE_NOTE(  5,  86),
  TUNE_NOTE(  5,  83),
  TUNE_NOTE(  5,  80),
  TUNE_NOTE(  5,  78),
  TUNE_NOTE(  5,  76),
  TUNE_NOTE(  5,  74),
  TUNE_NOTE(  5,  73),
  TUNE_NOTE(  5,  72),
  TUNE_NOTE(  5,  71),
  TUNE_NOTE(  5,  70),
  TUNE_NOTE(  5,  69),
  TUNE_NOTE(  5,  68),
  TUNE_NOTE(  5,  67),
  TUNE_NOTE(  5,  66),
  TUNE_NOTE(  5,  65),
  TUNE_NOTE(  5,  64),
  TUNE_NOTE(  5,  63),
  TUNE_NOTE(  5,  62),
  TUNE_NOTE(  5,  61),
  TUNE_NOTE(  5,  60),
  TUNE_NOTE(  5,  59),
  TUNE_NOTE(  5,  58),
  TUNE_STOP()
};
const TuneDefinition tune_screen_unfrozen = {
  strtext("Unfrozen"), 0, elements_screen_unfrozen
};

static const TuneElement elements_wrap_down[] = {
  TUNE_NOTE(  6,  86),
  TUNE_NOTE(  6,  74),
  TUNE_NOTE(  6,  62),
  TUNE_NOTE( 10,  50),
  TUNE_STOP()
};
const TuneDefinition tune_wrap_down = {
  NULL, TUNE_TACTILE(20,BRL_DOT1|BRL_DOT2|BRL_DOT3|BRL_DOT7), elements_wrap_down
};

static const TuneElement elements_wrap_up[] = {
  TUNE_NOTE(  6,  50),
  TUNE_NOTE(  6,  62),
  TUNE_NOTE(  6,  74),
  TUNE_NOTE( 10,  86),
  TUNE_STOP()
};
const TuneDefinition tune_wrap_up = {
  NULL, TUNE_TACTILE(20,BRL_DOT4|BRL_DOT5|BRL_DOT6|BRL_DOT8), elements_wrap_up
};

static const TuneElement elements_skip_first[] = {
  TUNE_REST( 40),
  TUNE_NOTE(  4,  62),
  TUNE_NOTE(  6,  67),
  TUNE_NOTE(  8,  74),
  TUNE_REST( 25),
  TUNE_STOP()
};
const TuneDefinition tune_skip_first = {
  NULL, TUNE_TACTILE(30,BRL_DOT1|BRL_DOT4|BRL_DOT7|BRL_DOT8), elements_skip_first
};

static const TuneElement elements_skip[] = {
  TUNE_NOTE( 10,  74),
  TUNE_REST( 18),
  TUNE_STOP()
};
const TuneDefinition tune_skip = {
  NULL, 0, elements_skip
};

static const TuneElement elements_skip_more[] = {
  TUNE_NOTE( 20,  73),
  TUNE_REST(  1),
  TUNE_STOP()
};
const TuneDefinition tune_skip_more = {
  NULL, 0, elements_skip_more
};

static const TuneElement elements_bounce[] = {
  TUNE_NOTE(  6,  98),
  TUNE_NOTE(  6,  86),
  TUNE_NOTE(  6,  74),
  TUNE_NOTE(  6,  62),
  TUNE_NOTE( 10,  50),
  TUNE_STOP()
};
const TuneDefinition tune_bounce = {
  NULL, TUNE_TACTILE(50,BRL_DOT1|BRL_DOT2|BRL_DOT3|BRL_DOT4|BRL_DOT5|BRL_DOT6|BRL_DOT7|BRL_DOT8), elements_bounce
};

static const TuneElement elements_routing_started[] = {
  TUNE_NOTE( 10,  55),
  TUNE_REST( 60),
  TUNE_NOTE( 15,  60),
  TUNE_STOP()
};
const TuneDefinition tune_routing_started = {
  NULL, 0, elements_routing_started
};

static const TuneElement elements_routing_succeeded[] = {
  TUNE_NOTE( 60,  64),
  TUNE_NOTE( 20,  76),
  TUNE_STOP()
};
const TuneDefinition tune_routing_succeeded = {
  NULL, 0, elements_routing_succeeded
};

static const TuneElement elements_routing_failed[] = {
  TUNE_NOTE( 80,  80),
  TUNE_NOTE( 90,  79),
  TUNE_NOTE(100,  78),
  TUNE_NOTE(100,  77),
  TUNE_REST( 20),
  TUNE_NOTE(100,  77),
  TUNE_REST( 20),
  TUNE_NOTE(150,  77),
  TUNE_STOP()
};
const TuneDefinition tune_routing_failed = {
  NULL, 0, elements_routing_failed
};

static const NoteMethods *noteMethods = NULL;
static NoteDevice *noteDevice = NULL;
static unsigned int closeTimer = 0;
static int openErrorLevel = LOG_ERR;

void
suppressTuneDeviceOpenErrors (void) {
  openErrorLevel = LOG_DEBUG;
}

static int
openTuneDevice (void) {
  if (noteDevice) return 1;
  if (!noteMethods) return 0;
  return (noteDevice = noteMethods->construct(openErrorLevel)) != NULL;
}

void
closeTuneDevice (int force) {
  if (force) {
    closeTimer = 0;
  } else if (closeTimer) {
    closeTimer--;
  }

  if (!closeTimer) {
    if (noteDevice) {
      noteMethods->destruct(noteDevice);
      noteDevice = NULL;
    }
  }
}
 
int
setTuneDevice (TuneDevice device) {
  const NoteMethods *methods;

  switch (device) {
    default:
      methods = NULL;
      break;

#ifdef ENABLE_BEEPER_SUPPORT
    case tdBeeper:
      methods = &beeperMethods;
      break;
#endif /* ENABLE_BEEPER_SUPPORT */

#ifdef ENABLE_PCM_SUPPORT
    case tdPcm:
      methods = &pcmMethods;
      break;
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
    case tdMidi:
      methods = &midiMethods;
      break;
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_FM_SUPPORT
    case tdFm:
      methods = &fmMethods;
      break;
#endif /* ENABLE_FM_SUPPORT */
  }
  if (!methods) return 0;

  if (methods != noteMethods) {
    closeTuneDevice(1);
    noteMethods = methods;
  }

  return 1;
}

void
playTune (const TuneDefinition *tune) {
  int tunePlayed = 0;

  if (prefs.alertTunes && tune->elements) {
    if (openTuneDevice()) {
      const TuneElement *element = tune->elements;

      tunePlayed = 1;
      closeTimer = 2000 / updateInterval;

      while (element->duration) {
        if (!noteMethods->play(noteDevice, element->note, element->duration)) {
          tunePlayed = 0;
          break;
        }

        ++element;
      }

      noteMethods->flush(noteDevice);
    }
  }

  if (!tunePlayed) {
    if (prefs.alertDots && tune->tactile) {
      unsigned char dots = tune->tactile & 0XFF;
      unsigned char duration = tune->tactile >> 8;
      showDotPattern(dots, duration);
    } else if (prefs.alertMessages && tune->message) {
      message(NULL, gettext(tune->message), 0);
    }
  }
}
