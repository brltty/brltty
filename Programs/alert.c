/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include "alert.h"
#include "prefs.h"
#include "tune.h"
#include "message.h"
#include "brl_dots.h"

static const NoteElement tuneBrailleOn[] = {
  NOTE_PLAY( 60,  64),
  NOTE_PLAY(100,  69),
  NOTE_STOP()
};

static const NoteElement tuneBrailleOff[] = {
  NOTE_PLAY( 60,  64),
  NOTE_PLAY( 60,  57),
  NOTE_STOP()
};

static const NoteElement tuneCommandDone[] = {
  NOTE_PLAY( 40,  74),
  NOTE_REST( 30),
  NOTE_PLAY( 40,  74),
  NOTE_REST( 40),
  NOTE_PLAY(140,  74),
  NOTE_REST( 20),
  NOTE_PLAY( 50,  79),
  NOTE_STOP()
};

static const NoteElement tuneCommandRejected[] = {
  NOTE_PLAY(100,  78),
  NOTE_STOP()
};

static const NoteElement tuneMarkSet[] = {
  NOTE_PLAY( 20,  83),
  NOTE_PLAY( 15,  81),
  NOTE_PLAY( 15,  79),
  NOTE_PLAY( 25,  84),
  NOTE_STOP()
};

static const NoteElement tuneClipboardBegin[] = {
  NOTE_PLAY( 40,  74),
  NOTE_PLAY( 20,  86),
  NOTE_STOP()
};

static const NoteElement tuneClipboardEnd[] = {
  NOTE_PLAY( 50,  86),
  NOTE_PLAY( 30,  74),
  NOTE_STOP()
};

static const NoteElement tuneNoChange[] = {
  NOTE_PLAY( 30,  79),
  NOTE_REST( 30),
  NOTE_PLAY( 30,  79),
  NOTE_REST( 30),
  NOTE_PLAY( 30,  79),
  NOTE_STOP()
};

static const NoteElement tuneToggleOn[] = {
  NOTE_PLAY( 30,  74),
  NOTE_REST( 30),
  NOTE_PLAY( 30,  79),
  NOTE_REST( 30),
  NOTE_PLAY( 30,  86),
  NOTE_STOP()
};

static const NoteElement tuneToggleOff[] = {
  NOTE_PLAY( 30,  86),
  NOTE_REST( 30),
  NOTE_PLAY( 30,  79),
  NOTE_REST( 30),
  NOTE_PLAY( 30,  74),
  NOTE_STOP()
};

static const NoteElement tuneCursorLinked[] = {
  NOTE_PLAY(  7,  80),
  NOTE_PLAY(  7,  79),
  NOTE_PLAY( 12,  76),
  NOTE_STOP()
};

static const NoteElement tuneCursorUnlinked[] = {
  NOTE_PLAY(  7,  78),
  NOTE_PLAY(  7,  79),
  NOTE_PLAY( 20,  83),
  NOTE_STOP()
};

static const NoteElement tuneScreenFrozen[] = {
  NOTE_PLAY(  5,  58),
  NOTE_PLAY(  5,  59),
  NOTE_PLAY(  5,  60),
  NOTE_PLAY(  5,  61),
  NOTE_PLAY(  5,  62),
  NOTE_PLAY(  5,  63),
  NOTE_PLAY(  5,  64),
  NOTE_PLAY(  5,  65),
  NOTE_PLAY(  5,  66),
  NOTE_PLAY(  5,  67),
  NOTE_PLAY(  5,  68),
  NOTE_PLAY(  5,  69),
  NOTE_PLAY(  5,  70),
  NOTE_PLAY(  5,  71),
  NOTE_PLAY(  5,  72),
  NOTE_PLAY(  5,  73),
  NOTE_PLAY(  5,  74),
  NOTE_PLAY(  5,  76),
  NOTE_PLAY(  5,  78),
  NOTE_PLAY(  5,  80),
  NOTE_PLAY(  5,  83),
  NOTE_PLAY(  5,  86),
  NOTE_PLAY(  5,  90),
  NOTE_PLAY(  5,  95),
  NOTE_STOP()
};

static const NoteElement tuneScreenUnfrozen[] = {
  NOTE_PLAY(  5,  95),
  NOTE_PLAY(  5,  90),
  NOTE_PLAY(  5,  86),
  NOTE_PLAY(  5,  83),
  NOTE_PLAY(  5,  80),
  NOTE_PLAY(  5,  78),
  NOTE_PLAY(  5,  76),
  NOTE_PLAY(  5,  74),
  NOTE_PLAY(  5,  73),
  NOTE_PLAY(  5,  72),
  NOTE_PLAY(  5,  71),
  NOTE_PLAY(  5,  70),
  NOTE_PLAY(  5,  69),
  NOTE_PLAY(  5,  68),
  NOTE_PLAY(  5,  67),
  NOTE_PLAY(  5,  66),
  NOTE_PLAY(  5,  65),
  NOTE_PLAY(  5,  64),
  NOTE_PLAY(  5,  63),
  NOTE_PLAY(  5,  62),
  NOTE_PLAY(  5,  61),
  NOTE_PLAY(  5,  60),
  NOTE_PLAY(  5,  59),
  NOTE_PLAY(  5,  58),
  NOTE_STOP()
};

static const NoteElement tuneFreezeReminder[] = {
  NOTE_PLAY( 50,  60),
  NOTE_REST( 30),
  NOTE_PLAY( 50,  60),
  NOTE_STOP()
};

static const NoteElement tuneWrapDown[] = {
  NOTE_PLAY(  6,  86),
  NOTE_PLAY(  6,  74),
  NOTE_PLAY(  6,  62),
  NOTE_PLAY( 10,  50),
  NOTE_STOP()
};

static const NoteElement tuneWrapUp[] = {
  NOTE_PLAY(  6,  50),
  NOTE_PLAY(  6,  62),
  NOTE_PLAY(  6,  74),
  NOTE_PLAY( 10,  86),
  NOTE_STOP()
};

static const NoteElement tuneSkipFirst[] = {
  NOTE_REST( 40),
  NOTE_PLAY(  4,  62),
  NOTE_PLAY(  6,  67),
  NOTE_PLAY(  8,  74),
  NOTE_REST( 25),
  NOTE_STOP()
};

static const NoteElement tuneSkip[] = {
  NOTE_PLAY( 10,  74),
  NOTE_REST( 18),
  NOTE_STOP()
};

static const NoteElement tuneSkipMore[] = {
  NOTE_PLAY( 20,  73),
  NOTE_REST(  1),
  NOTE_STOP()
};

static const NoteElement tuneBounce[] = {
  NOTE_PLAY(  6,  98),
  NOTE_PLAY(  6,  86),
  NOTE_PLAY(  6,  74),
  NOTE_PLAY(  6,  62),
  NOTE_PLAY( 10,  50),
  NOTE_STOP()
};

static const NoteElement tuneRoutingStarted[] = {
  NOTE_PLAY( 10,  55),
  NOTE_REST( 60),
  NOTE_PLAY( 15,  60),
  NOTE_STOP()
};

static const NoteElement tuneRoutingSucceeded[] = {
  NOTE_PLAY( 60,  64),
  NOTE_PLAY( 20,  76),
  NOTE_STOP()
};

static const NoteElement tuneRoutingFailed[] = {
  NOTE_PLAY( 80,  80),
  NOTE_PLAY( 90,  79),
  NOTE_PLAY(100,  78),
  NOTE_PLAY(100,  77),
  NOTE_REST( 20),
  NOTE_PLAY(100,  77),
  NOTE_REST( 20),
  NOTE_PLAY(150,  77),
  NOTE_STOP()
};

static const NoteElement tuneModifierNext[] = {
  NOTE_PLAY( 60,  72),
  NOTE_PLAY( 60,  76),
  NOTE_PLAY( 90,  79),
  NOTE_STOP()
};

static const NoteElement tuneModifierOn[] = {
  NOTE_PLAY( 60,  72),
  NOTE_PLAY( 60,  76),
  NOTE_PLAY( 60,  79),
  NOTE_PLAY( 90,  84),
  NOTE_STOP()
};

static const NoteElement tuneModifierOff[] = {
  NOTE_PLAY( 60,  84),
  NOTE_PLAY( 60,  79),
  NOTE_PLAY( 60,  76),
  NOTE_PLAY( 90,  72),
  NOTE_STOP()
};

typedef struct {
  unsigned char duration;
  BrlDots pattern;
} TactileAlert;

typedef struct {
  const NoteElement *tune;
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

  [ALERT_MODIFIER_NEXT] = {
    .tune = tuneModifierNext
  },

  [ALERT_MODIFIER_ON] = {
    .tune = tuneModifierOn
  },

  [ALERT_MODIFIER_OFF] = {
    .tune = tuneModifierOff
  },
};

void
alert (AlertIdentifier identifier) {
  if ((identifier >= 0) && (identifier < ARRAY_COUNT(alertTable))) {
    const AlertEntry *alert = &alertTable[identifier];

    if (prefs.alertTunes && alert->tune) {
      tunePlayNotes(alert->tune);
    } else if (prefs.alertDots && alert->tactile.duration) {
      showDotPattern(alert->tactile.pattern, alert->tactile.duration);
    } else if (prefs.alertMessages && alert->message) {
      message(NULL, gettext(alert->message), 0);
    }
  }
}
