/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#include "alert.h"
#include "prefs.h"
#include "tune.h"
#include "message.h"
#include "brl_dots.h"

static const TuneElement tuneBrailleOn[] = {
  TUNE_NOTE( 60,  64),
  TUNE_NOTE(100,  69),
  TUNE_STOP()
};

static const TuneElement tuneBrailleOff[] = {
  TUNE_NOTE( 60,  64),
  TUNE_NOTE( 60,  57),
  TUNE_STOP()
};

static const TuneElement tuneCommandDone[] = {
  TUNE_NOTE( 40,  74),
  TUNE_REST( 30),
  TUNE_NOTE( 40,  74),
  TUNE_REST( 40),
  TUNE_NOTE(140,  74),
  TUNE_REST( 20),
  TUNE_NOTE( 50,  79),
  TUNE_STOP()
};

static const TuneElement tuneCommandRejected[] = {
  TUNE_NOTE(100,  78),
  TUNE_STOP()
};

static const TuneElement tuneMarkSet[] = {
  TUNE_NOTE( 20,  83),
  TUNE_NOTE( 15,  81),
  TUNE_NOTE( 15,  79),
  TUNE_NOTE( 25,  84),
  TUNE_STOP()
};

static const TuneElement tuneClipboardBegin[] = {
  TUNE_NOTE( 40,  74),
  TUNE_NOTE( 20,  86),
  TUNE_STOP()
};

static const TuneElement tuneClipboardEnd[] = {
  TUNE_NOTE( 50,  86),
  TUNE_NOTE( 30,  74),
  TUNE_STOP()
};

static const TuneElement tuneNoChange[] = {
  TUNE_NOTE( 30,  79),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  79),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  79),
  TUNE_STOP()
};

static const TuneElement tuneToggleOn[] = {
  TUNE_NOTE( 30,  74),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  79),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  86),
  TUNE_STOP()
};

static const TuneElement tuneToggleOff[] = {
  TUNE_NOTE( 30,  86),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  79),
  TUNE_REST( 30),
  TUNE_NOTE( 30,  74),
  TUNE_STOP()
};

static const TuneElement tuneCursorLinked[] = {
  TUNE_NOTE(  7,  80),
  TUNE_NOTE(  7,  79),
  TUNE_NOTE( 12,  76),
  TUNE_STOP()
};

static const TuneElement tuneCursorUnlinked[] = {
  TUNE_NOTE(  7,  78),
  TUNE_NOTE(  7,  79),
  TUNE_NOTE( 20,  83),
  TUNE_STOP()
};

static const TuneElement tuneScreenFrozen[] = {
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

static const TuneElement tuneScreenUnfrozen[] = {
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

static const TuneElement tuneFreezeReminder[] = {
  TUNE_NOTE( 50,  60),
  TUNE_REST( 30),
  TUNE_NOTE( 50,  60),
  TUNE_STOP()
};

static const TuneElement tuneWrapDown[] = {
  TUNE_NOTE(  6,  86),
  TUNE_NOTE(  6,  74),
  TUNE_NOTE(  6,  62),
  TUNE_NOTE( 10,  50),
  TUNE_STOP()
};

static const TuneElement tuneWrapUp[] = {
  TUNE_NOTE(  6,  50),
  TUNE_NOTE(  6,  62),
  TUNE_NOTE(  6,  74),
  TUNE_NOTE( 10,  86),
  TUNE_STOP()
};

static const TuneElement tuneSkipFirst[] = {
  TUNE_REST( 40),
  TUNE_NOTE(  4,  62),
  TUNE_NOTE(  6,  67),
  TUNE_NOTE(  8,  74),
  TUNE_REST( 25),
  TUNE_STOP()
};

static const TuneElement tuneSkip[] = {
  TUNE_NOTE( 10,  74),
  TUNE_REST( 18),
  TUNE_STOP()
};

static const TuneElement tuneSkipMore[] = {
  TUNE_NOTE( 20,  73),
  TUNE_REST(  1),
  TUNE_STOP()
};

static const TuneElement tuneBounce[] = {
  TUNE_NOTE(  6,  98),
  TUNE_NOTE(  6,  86),
  TUNE_NOTE(  6,  74),
  TUNE_NOTE(  6,  62),
  TUNE_NOTE( 10,  50),
  TUNE_STOP()
};

static const TuneElement tuneRoutingStarted[] = {
  TUNE_NOTE( 10,  55),
  TUNE_REST( 60),
  TUNE_NOTE( 15,  60),
  TUNE_STOP()
};

static const TuneElement tuneRoutingSucceeded[] = {
  TUNE_NOTE( 60,  64),
  TUNE_NOTE( 20,  76),
  TUNE_STOP()
};

static const TuneElement tuneRoutingFailed[] = {
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

typedef struct {
  unsigned char duration;
  BrlDots pattern;
} TactileAlert;

typedef struct {
  const TuneElement *tune;
  const char *message;
  TactileAlert tactile;
} AlertEntry;

#define ALERT_TACTILE(d,p) {.duration=(d), .pattern=(p)}

static const AlertEntry alertTable[] = {
  [ALERT_BRAILLE_ON] = {
    .tune = tuneBrailleOn
  },

  [ALERT_BRAILLE_OFF] = {
    .tune = tuneBrailleOff
  },

  [ALERT_COMMAND_DONE] = {
    .message = strtext("Done"),
    .tune = tuneCommandDone
  },

  [ALERT_COMMAND_REJECTED] = {
    .tactile = ALERT_TACTILE(50, BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_6),
    .tune = tuneCommandRejected
  },

  [ALERT_MARK_SET] = {
    .tune = tuneMarkSet
  },

  [ALERT_CLIPBOARD_BEGIN] = {
    .tune = tuneClipboardBegin
  },

  [ALERT_CLIPBOARD_END] = {
    .tune = tuneClipboardEnd
  },

  [ALERT_NO_CHANGE] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_5 | BRL_DOT_6),
    .tune = tuneNoChange
  },

  [ALERT_TOGGLE_ON] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5),
    .tune = tuneToggleOn
  },

  [ALERT_TOGGLE_OFF] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_3 | BRL_DOT_7 | BRL_DOT_6 | BRL_DOT_8),
    .tune = tuneToggleOff
  },

  [ALERT_CURSOR_LINKED] = {
    .tune = tuneCursorLinked
  },

  [ALERT_CURSOR_UNLINKED] = {
    .tune = tuneCursorUnlinked
  },

  [ALERT_SCREEN_FROZEN] = {
    .message = strtext("Frozen"),
    .tune = tuneScreenFrozen
  },

  [ALERT_SCREEN_UNFROZEN] = {
    .message = strtext("Unfrozen"),
    .tune = tuneScreenUnfrozen
  },

  [ALERT_FREEZE_REMINDER] = {
    .tune = tuneFreezeReminder
  },

  [ALERT_WRAP_DOWN] = {
    .tactile = ALERT_TACTILE(20, BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6 | BRL_DOT_8),
    .tune = tuneWrapDown
  },

  [ALERT_WRAP_UP] = {
    .tactile = ALERT_TACTILE(20, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_7),
    .tune = tuneWrapUp
  },

  [ALERT_SKIP_FIRST] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_1 | BRL_DOT_4 | BRL_DOT_7 | BRL_DOT_8),
    .tune = tuneSkipFirst
  },

  [ALERT_SKIP] = {
    .tune = tuneSkip
  },

  [ALERT_SKIP_MORE] = {
    .tune = tuneSkipMore
  },

  [ALERT_BOUNCE] = {
    .tactile = ALERT_TACTILE(50, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6 | BRL_DOT_7 | BRL_DOT_8),
    .tune = tuneBounce
  },

  [ALERT_ROUTING_STARTED] = {
    .tune = tuneRoutingStarted
  },

  [ALERT_ROUTING_SUCCEEDED] = {
    .tune = tuneRoutingSucceeded
  },

  [ALERT_ROUTING_FAILED] = {
    .tune = tuneRoutingFailed
  },
};

void
alert (AlertIdentifier identifier) {
  if (identifier < ARRAY_COUNT(alertTable)) {
    const AlertEntry *alert = &alertTable[identifier];

    if (!(prefs.alertTunes && alert->tune && playTune(alert->tune))) {
      if (prefs.alertDots && alert->tactile.duration) {
        showDotPattern(alert->tactile.pattern, alert->tactile.duration);
      } else if (prefs.alertMessages && alert->message) {
        message(NULL, gettext(alert->message), 0);
      }
    }
  }
}
