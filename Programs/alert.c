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
#include "program.h"
#include "prefs.h"
#include "tune.h"
#include "tune_build.h"
#include "message.h"
#include "brl_dots.h"

typedef struct {
  unsigned char duration;
  BrlDots pattern;
} TactileAlert;

typedef struct {
  const char *tune;
  const char *message;
  TactileAlert tactile;
} AlertEntry;

#define ALERT_TACTILE(d,p) {.duration=(d), .pattern=(p)}

static const AlertEntry alertTable[] = {
  [ALERT_BRAILLE_ON] = {
    .tune = "n64@60 n69@100"
  },

  [ALERT_BRAILLE_OFF] = {
    .tune = "n64@60 n57@60"
  },

  [ALERT_COMMAND_DONE] = {
    .message = strtext("Done"),
    .tune = "n74@40 r30 n74@40 r40 n74@140 r20 n79@50"
  },

  [ALERT_COMMAND_REJECTED] = {
    .tactile = ALERT_TACTILE(50, BRL_DOT_1 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_6),
    .tune = "n78@100"
  },

  [ALERT_MARK_SET] = {
    .tune = "n83@20 n81@15 n79@15 n84@25"
  },

  [ALERT_CLIPBOARD_BEGIN] = {
    .tune = "n74@40 n86@20"
  },

  [ALERT_CLIPBOARD_END] = {
    .tune = "n86@50 n74@30"
  },

  [ALERT_NO_CHANGE] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_5 | BRL_DOT_6),
    .tune = "n79@30 r30 n79@30 r30 n79@30"
  },

  [ALERT_TOGGLE_ON] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_4 | BRL_DOT_5),
    .tune = "n74@30 r30 n79@30 r30 n86@30"
  },

  [ALERT_TOGGLE_OFF] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_3 | BRL_DOT_7 | BRL_DOT_6 | BRL_DOT_8),
    .tune = "n86@30 r30 n79@30 r30 n74@30"
  },

  [ALERT_CURSOR_LINKED] = {
    .tune = "n80@7 n79@7 n76@12"
  },

  [ALERT_CURSOR_UNLINKED] = {
    .tune = "n78@7 n79@7 n83@20"
  },

  [ALERT_SCREEN_FROZEN] = {
    .message = strtext("Frozen"),
    .tune = "n58@5 n59@5 n60@5 n61@5 n62@5 n63@5 n64@5 n65@5 n66@5 n67@5 n68@5 n69@5 n70@5 n71@5 n72@5 n73@5 n74@5 n76@5 n78@5 n80@5 n83@5 n86@5 n90@5 n95@5"
  },

  [ALERT_SCREEN_UNFROZEN] = {
    .message = strtext("Unfrozen"),
    .tune = "n95@5 n90@5 n86@5 n83@5 n80@5 n78@5 n76@5 n74@5 n73@5 n72@5 n71@5 n70@5 n69@5 n68@5 n67@5 n66@5 n65@5 n64@5 n63@5 n62@5 n61@5 n60@5 n59@5 n58@5"
  },

  [ALERT_FREEZE_REMINDER] = {
    .tune = "n60@50 r30 n60@50"
  },

  [ALERT_WRAP_DOWN] = {
    .tactile = ALERT_TACTILE(20, BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6 | BRL_DOT_8),
    .tune = "n86@6 n74@6 n62@6 n50@10"
  },

  [ALERT_WRAP_UP] = {
    .tactile = ALERT_TACTILE(20, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_7),
    .tune = "n50@6 n62@6 n74@6 n86@10"
  },

  [ALERT_SKIP_FIRST] = {
    .tactile = ALERT_TACTILE(30, BRL_DOT_1 | BRL_DOT_4 | BRL_DOT_7 | BRL_DOT_8),
    .tune = "r40 n62@4 n67@6 n74@8 r25"
  },

  [ALERT_SKIP] = {
    .tune = "n74@10 r18"
  },

  [ALERT_SKIP_MORE] = {
    .tune = "n73@20 r1"
  },

  [ALERT_BOUNCE] = {
    .tactile = ALERT_TACTILE(50, BRL_DOT_1 | BRL_DOT_2 | BRL_DOT_3 | BRL_DOT_4 | BRL_DOT_5 | BRL_DOT_6 | BRL_DOT_7 | BRL_DOT_8),
    .tune = "n98@6 n86@6 n74@6 n62@6 n50@10"
  },

  [ALERT_ROUTING_STARTED] = {
    .tune = "n55@10 r60 n60@15"
  },

  [ALERT_ROUTING_SUCCEEDED] = {
    .tune = "n64@60 n76@20"
  },

  [ALERT_ROUTING_FAILED] = {
    .tune = "n80@80 n79@90 n78@100 n77@100 r20 n77@100 r20 n77@150"
  },

  [ALERT_MODIFIER_NEXT] = {
    .tune = "n72@60 n76@60 n79@90"
  },

  [ALERT_MODIFIER_ON] = {
    .tune = "n72@60 n76@60 n79@60 n84@90"
  },

  [ALERT_MODIFIER_OFF] = {
    .tune = "n84@60 n79@60 n76@60 n72@90"
  },

  [ALERT_CONSOLE_BELL] = {
    .message = strtext("Console Bell"),
    .tune = "n78@100"
  },
};

static ToneElement *tuneTable[ARRAY_COUNT(alertTable)] = {NULL};
static TuneBuilder *tuneBuilder = NULL;
static ToneElement emptyTune[] = {TONE_STOP()};

static void
exitAlertTunes (void *data) {
  ToneElement **tune = tuneTable;
  ToneElement **end = tune + ARRAY_COUNT(tuneTable);

  while (tune < end) {
    if (*tune) {
      if (*tune != emptyTune) free(*tune);
      *tune = NULL;
    }

    tune += 1;
  }

  if (tuneBuilder) {
    destroyTuneBuilder(tuneBuilder);
    tuneBuilder = NULL;
  }
}

static TuneBuilder *
getTuneBuilder (void) {
  if (!tuneBuilder) {
    if (!(tuneBuilder = newTuneBuilder())) {
      return NULL;
    }

    onProgramExit("alert-tunes", exitAlertTunes, NULL);
  }

  return tuneBuilder;
}

void
alert (AlertIdentifier identifier) {
  if (identifier < ARRAY_COUNT(alertTable)) {
    const AlertEntry *alert = &alertTable[identifier];

    if (prefs.alertTunes && alert->tune && *alert->tune) {
      ToneElement **tune = &tuneTable[identifier];

      if (!*tune) {
        TuneBuilder *tb = getTuneBuilder();

        if (tb) {
          setTuneSourceName(tuneBuilder, "alert");
          setTuneSourceIndex(tb, identifier);

          if (parseTuneString(tb, alert->tune)) {
            *tune = getTune(tb);
          }

          resetTuneBuilder(tb);
        }

        if (!*tune) *tune = emptyTune;
      }

      tunePlayTones(*tune);
    } else if (prefs.alertDots && alert->tactile.duration) {
      showDotPattern(alert->tactile.pattern, alert->tactile.duration);
    } else if (prefs.alertMessages && alert->message) {
      message(NULL, gettext(alert->message), 0);
    }
  }
}
