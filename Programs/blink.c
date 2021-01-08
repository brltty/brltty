/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2021 by The BRLTTY Developers.
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

#include "prologue.h"

#include "blink.h"
#include "prefs.h"
#include "async_alarm.h"
#include "update.h"
#include "core.h"

struct BlinkDescriptorStruct {
  const char *const name;
  const unsigned char *const isEnabled;
  unsigned char *const visibleTime;
  unsigned char *const invisibleTime;

  unsigned isRequired:1;
  unsigned isVisible:1;
  AsyncHandle alarmHandle;
};

BlinkDescriptor screenCursorBlinkDescriptor = {
  .name = "screen cursor",
  .isEnabled = &prefs.blinkingScreenCursor,
  .visibleTime = &prefs.screenCursorVisibleTime,
  .invisibleTime = &prefs.screenCursorInvisibleTime
};

BlinkDescriptor attributesUnderlineBlinkDescriptor = {
  .name = "attributes underline",
  .isEnabled = &prefs.blinkingAttributes,
  .visibleTime = &prefs.attributesVisibleTime,
  .invisibleTime = &prefs.attributesInvisibleTime
};

BlinkDescriptor uppercaseLettersBlinkDescriptor = {
  .name = "uppercase letters",
  .isEnabled = &prefs.blinkingCapitals,
  .visibleTime = &prefs.capitalsVisibleTime,
  .invisibleTime = &prefs.capitalsInvisibleTime
};

BlinkDescriptor speechCursorBlinkDescriptor = {
  .name = "speech cursor",
  .isEnabled = &prefs.blinkingSpeechCursor,
  .visibleTime = &prefs.speechCursorVisibleTime,
  .invisibleTime = &prefs.speechCursorInvisibleTime
};

static BlinkDescriptor *const blinkDescriptors[] = {
  &screenCursorBlinkDescriptor,
  &attributesUnderlineBlinkDescriptor,
  &uppercaseLettersBlinkDescriptor,
  &speechCursorBlinkDescriptor,
  NULL
};

static int
getBlinkVisibleTime (BlinkDescriptor *blink) {
  return PREFS2MSECS(*blink->visibleTime);
}

static int
getBlinkInvisibleTime (BlinkDescriptor *blink) {
  return PREFS2MSECS(*blink->invisibleTime);
}

int
getBlinkPeriod (BlinkDescriptor *blink) {
  return getBlinkVisibleTime(blink) + getBlinkInvisibleTime(blink);
}

int
getBlinkPercentage (BlinkDescriptor *blink) {
  return (getBlinkVisibleTime(blink) * 100) / getBlinkPeriod(blink);
}

int
setBlinkProperties (BlinkDescriptor *blink, int period, int percentage) {
  if (period < 1) return 0;
  period = MSECS2PREFS(period);
  if (period > UINT8_MAX) return 0;

  if (percentage < 0) return 0;
  if (percentage > 100) return 0;

  int visible = (period * percentage) / 100;
  int invisible = period - visible;

  *blink->visibleTime = visible;
  *blink->invisibleTime = invisible;
  return 1;
}

int
isBlinkVisible (const BlinkDescriptor *blink) {
  if (!*blink->isEnabled) return 1;
  return blink->isVisible;
}

static int
getBlinkDuration (const BlinkDescriptor *blink) {
  return PREFS2MSECS(blink->isVisible? *blink->visibleTime: *blink->invisibleTime);
}

void
setBlinkState (BlinkDescriptor *blink, int visible) {
  int changed = visible != blink->isVisible;

  blink->isVisible = visible;

  if (blink->alarmHandle) {
    asyncResetAlarmIn(blink->alarmHandle, getBlinkDuration(blink));
    if (changed) scheduleUpdate("blink state set");
  }
}

static void setBlinkAlarm (BlinkDescriptor *blink);

ASYNC_ALARM_CALLBACK(handleBlinkAlarm) {
  BlinkDescriptor *blink = parameters->data;

  asyncDiscardHandle(blink->alarmHandle);
  blink->alarmHandle = NULL;

  blink->isVisible = !blink->isVisible;
  setBlinkAlarm(blink);
  scheduleUpdate("blink state changed");
}

static void
setBlinkAlarm (BlinkDescriptor *blink) {
  asyncNewRelativeAlarm(&blink->alarmHandle, getBlinkDuration(blink), handleBlinkAlarm, blink);
}

static void
forEachBlinkDescriptor (void (*handleBlinkDescriptor) (BlinkDescriptor *blink)) {
  BlinkDescriptor *const *blink = blinkDescriptors;

  while (*blink) handleBlinkDescriptor(*blink++);
}

static void
unrequireBlinkDescriptor (BlinkDescriptor *blink) {
  blink->isRequired = 0;
}

void
unrequireAllBlinkDescriptors (void) {
  forEachBlinkDescriptor(unrequireBlinkDescriptor);
}

void
requireBlinkDescriptor (BlinkDescriptor *blink) {
  blink->isRequired = 1;
}

static void
stopBlinkDescriptor (BlinkDescriptor *blink) {
  if (blink->alarmHandle) {
    asyncCancelRequest(blink->alarmHandle);
    blink->alarmHandle = NULL;
  }
}

void
stopAllBlinkDescriptors (void) {
  forEachBlinkDescriptor(stopBlinkDescriptor);
}

static void
resetBlinkDescriptor (BlinkDescriptor *blink) {
  if (!(*blink->isEnabled && blink->isRequired)) {
    stopBlinkDescriptor(blink);
  } else if (!blink->alarmHandle) {
    setBlinkAlarm(blink);
  }
}

void
resetAllBlinkDescriptors (void) {
  forEachBlinkDescriptor(resetBlinkDescriptor);
}
