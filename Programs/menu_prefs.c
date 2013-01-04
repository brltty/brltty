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

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "menu.h"
#include "prefs.h"
#include "statdefs.h"
#include "brltty.h"
#include "ttb.h"
#include "atb.h"
#include "ctb.h"
#include "tunes.h"

#define NAME(name) static const MenuString itemName = {.label=name}
#define ITEM(new) MenuItem *item = (new); if (!item) goto noItem
#define TEST(property) setMenuItemTester(item, test##property)
#define CHANGED(setting) setMenuItemChanged(item, changed##setting)
#define SUBMENU(variable, parent, name) \
  NAME(name); \
  Menu *variable = newSubmenuMenuItem(parent, &itemName); \
  if (!variable) goto noItem

static int
testAdvancedSubmenu (void) {
  return prefs.showAdvancedSubmenus;
}

static void
setAdvancedSubmenu (Menu *submenu) {
  Menu *parent = getMenuParent(submenu);

  if (parent) {
    unsigned int size = getMenuSize(parent);

    if (size) {
      MenuItem *item = getMenuItem(parent, size-1);

      if (item) setMenuItemTester(item, testAdvancedSubmenu);
    }
  }
}

static int
testSkipBlankWindows (void) {
  return prefs.skipBlankWindows;
}

static int
testSlidingWindow (void) {
  return prefs.slidingWindow;
}

static int
changedWindowOverlap (const MenuItem *item UNUSED, unsigned char setting) {
  if (setting >= textCount) return 0;
  reconfigureWindow();
  return 1;
}

static int
testAutorepeat (void) {
  return prefs.autorepeat;
}

static int
changedAutorepeat (const MenuItem *item UNUSED, unsigned char setting) {
  if (setting) resetAutorepeat();
  return 1;
}

static int
testShowCursor (void) {
  return prefs.showCursor;
}

static int
testBlinkingCursor (void) {
  return testShowCursor() && prefs.blinkingCursor;
}

static int
testShowAttributes (void) {
  return prefs.showAttributes;
}

static int
testBlinkingAttributes (void) {
  return testShowAttributes() && prefs.blinkingAttributes;
}

static int
testBlinkingCapitals (void) {
  return prefs.blinkingCapitals;
}

static int
testBrailleFirmness (void) {
  return brl.setFirmness != NULL;
}

static int
changedBrailleFirmness (const MenuItem *item UNUSED, unsigned char setting) {
  return setBrailleFirmness(&brl, setting);
}

static int
testBrailleSensitivity (void) {
  return brl.setSensitivity != NULL;
}

static int
changedBrailleSensitivity (const MenuItem *item UNUSED, unsigned char setting) {
  return setBrailleSensitivity(&brl, setting);
}

static int
testTunes (void) {
  return prefs.alertTunes;
}

static int
changedTuneDevice (const MenuItem *item UNUSED, unsigned char setting) {
  return setTuneDevice(setting);
}

#ifdef ENABLE_PCM_SUPPORT
static int
testTunesPcm (void) {
  return testTunes() && (prefs.tuneDevice == tdPcm);
}
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
static int
testTunesMidi (void) {
  return testTunes() && (prefs.tuneDevice == tdMidi);
}
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_FM_SUPPORT
static int
testTunesFm (void) {
  return testTunes() && (prefs.tuneDevice == tdFm);
}
#endif /* ENABLE_FM_SUPPORT */

#ifdef ENABLE_SPEECH_SUPPORT
static int
testSpeechVolume (void) {
  return speech->setVolume != NULL;
}

static int
changedSpeechVolume (const MenuItem *item UNUSED, unsigned char setting) {
  return setSpeechVolume(&spk, setting, !prefs.autospeak);
}

static int
testSpeechRate (void) {
  return speech->setRate != NULL;
}

static int
changedSpeechRate (const MenuItem *item UNUSED, unsigned char setting) {
  return setSpeechRate(&spk, setting, !prefs.autospeak);
}

static int
testSpeechPitch (void) {
  return speech->setPitch != NULL;
}

static int
changedSpeechPitch (const MenuItem *item UNUSED, unsigned char setting) {
  return setSpeechPitch(&spk, setting, !prefs.autospeak);
}

static int
testSpeechPunctuation (void) {
  return speech->setPunctuation != NULL;
}

static int
changedSpeechPunctuation (const MenuItem *item UNUSED, unsigned char setting) {
  return setSpeechPunctuation(&spk, setting, !prefs.autospeak);
}

static int
testAutospeak (void) {
  return prefs.autospeak;
}

static int
testShowSpeechCursor (void) {
  return prefs.showSpeechCursor;
}

static int
testBlinkingSpeechCursor (void) {
  return testShowSpeechCursor() && prefs.blinkingSpeechCursor;
}
#endif /* ENABLE_SPEECH_SUPPORT */

static int
testShowDate (void) {
  return prefs.datePosition != dpNone;
}

static int
testStatusPosition (void) {
  return !haveStatusCells();
}

static int
changedStatusPosition (const MenuItem *item UNUSED, unsigned char setting UNUSED) {
  reconfigureWindow();
  return 1;
}

static int
testStatusCount (void) {
  return testStatusPosition() && (prefs.statusPosition != spNone);
}

static int
changedStatusCount (const MenuItem *item UNUSED, unsigned char setting UNUSED) {
  reconfigureWindow();
  return 1;
}

static int
testStatusSeparator (void) {
  return testStatusCount();
}

static int
changedStatusSeparator (const MenuItem *item UNUSED, unsigned char setting UNUSED) {
  reconfigureWindow();
  return 1;
}

static int
testStatusField (unsigned char index) {
  return (haveStatusCells() || (prefs.statusPosition != spNone)) &&
         ((index == 0) || (prefs.statusFields[index-1] != sfEnd));
}

static int
changedStatusField (unsigned char index, unsigned char setting) {
  switch (setting) {
    case sfGeneric:
      if (index > 0) return 0;
      if (!haveStatusCells()) return 0;
      if (!braille->statusFields) return 0;
      if (*braille->statusFields != sfGeneric) return 0;

    case sfEnd:
      if (prefs.statusFields[index+1] != sfEnd) return 0;
      break;

    default:
      if ((index > 0) && (prefs.statusFields[index-1] == sfGeneric)) return 0;
      break;
  }

  reconfigureWindow();
  return 1;
}

#define STATUS_FIELD_HANDLERS(n) \
  static int testStatusField##n (void) { return testStatusField(n-1); } \
  static int changedStatusField##n (const MenuItem *item UNUSED, unsigned char setting) { return changedStatusField(n-1, setting); }
STATUS_FIELD_HANDLERS(1)
STATUS_FIELD_HANDLERS(2)
STATUS_FIELD_HANDLERS(3)
STATUS_FIELD_HANDLERS(4)
STATUS_FIELD_HANDLERS(5)
STATUS_FIELD_HANDLERS(6)
STATUS_FIELD_HANDLERS(7)
STATUS_FIELD_HANDLERS(8)
STATUS_FIELD_HANDLERS(9)
#undef STATUS_FIELD_HANDLERS

static int
changedTextTable (const MenuItem *item, unsigned char setting UNUSED) {
  return replaceTextTable(opt_tablesDirectory, getMenuItemValue(item));
}

static int
changedAttributesTable (const MenuItem *item, unsigned char setting UNUSED) {
  return replaceAttributesTable(opt_tablesDirectory, getMenuItemValue(item));
}

#ifdef ENABLE_CONTRACTED_BRAILLE
static int
testContractedBraille (void) {
  return prefs.textStyle == tsContractedBraille;
}

static int
changedContractionTable (const MenuItem *item, unsigned char setting UNUSED) {
  return loadContractionTable(getMenuItemValue(item));
}
#endif /* ENABLE_CONTRACTED_BRAILLE */

static int
testBrailleKeyTable (void) {
  return !!brl.keyTable;
}

static int
testKeyboardKeyTable (void) {
  return !!keyboardKeyTable;
}

static MenuItem *
newStatusFieldMenuItem (
  Menu *menu, unsigned char number,
  MenuItemTester *test, MenuItemChanged *changed
) {
  static const MenuString strings[] = {
    {.label=strtext("End")},
    {.label=strtext("Window Coordinates"), .comment=strtext("2 cells")},
    {.label=strtext("Window Column"), .comment=strtext("1 cell")},
    {.label=strtext("Window Row"), .comment=strtext("1 cell")},
    {.label=strtext("Cursor Coordinates"), .comment=strtext("2 cells")},
    {.label=strtext("Cursor Column"), .comment=strtext("1 cell")},
    {.label=strtext("Cursor Row"), .comment=strtext("1 cell")},
    {.label=strtext("Cursor and Window Column"), .comment=strtext("2 cells")},
    {.label=strtext("Cursor and Window Row"), .comment=strtext("2 cells")},
    {.label=strtext("Screen Number"), .comment=strtext("1 cell")},
    {.label=strtext("State Dots"), .comment=strtext("1 cell")},
    {.label=strtext("State Letter"), .comment=strtext("1 cell")},
    {.label=strtext("Time"), .comment=strtext("2 cells")},
    {.label=strtext("Alphabetic Window Coordinates"), .comment=strtext("1 cell")},
    {.label=strtext("Alphabetic Cursor Coordinates"), .comment=strtext("1 cell")},
    {.label=strtext("Generic")}
  };

  MenuString name = {
    .label = strtext("Status Field")
  };

  char *comment;
  {
    char buffer[0X3];
    snprintf(buffer, sizeof(buffer), "%u", number);
    name.comment = comment = strdup(buffer);
  }

  if (comment) {
    MenuItem *item = newEnumeratedMenuItem(menu, &prefs.statusFields[number-1], &name, strings);

    if (item) {
      setMenuItemTester(item, test);
      setMenuItemChanged(item, changed);
      return item;
    }

    free(comment);
  } else {
    logMallocError();
  }

  return NULL;
}

static MenuItem *
newTimeMenuItem (Menu *menu, unsigned char *setting, const MenuString *name) {
  return newNumericMenuItem(menu, setting, name, 1, 100, updateInterval/10);
}

#if defined(ENABLE_PCM_SUPPORT) || defined(ENABLE_MIDI_SUPPORT) || defined(ENABLE_FM_SUPPORT)
static MenuItem *
newVolumeMenuItem (Menu *menu, unsigned char *setting, const MenuString *name) {
  return newNumericMenuItem(menu, setting, name, 0, 100, 5);
}
#endif /* defined(ENABLE_PCM_SUPPORT) || defined(ENABLE_MIDI_SUPPORT) || defined(ENABLE_FM_SUPPORT) */

#ifdef ENABLE_MIDI_SUPPORT
MenuString *
makeMidiInstrumentMenuStrings (void) {
  MenuString *strings = malloc(midiInstrumentCount * sizeof(*strings));

  if (strings) {
    unsigned int instrument;

    for (instrument=0; instrument<midiInstrumentCount; instrument+=1) {
      MenuString *string = &strings[instrument];
      string->label = midiInstrumentTable[instrument];
      string->comment = midiGetInstrumentType(instrument);
    }
  }

  return strings;
}
#endif /* ENABLE_MIDI_SUPPORT */

static Menu *
makePreferencesMenu (void) {
  static const MenuString cursorStyles[] = {
    {.label=strtext("Underline"), .comment=strtext("dots 7 and 8")},
    {.label=strtext("Block"), .comment=strtext("all dots")},
    {.label=strtext("Lower Left Dot"), .comment=strtext("dot 7")},
    {.label=strtext("Lower Right Dot"), .comment=strtext("dot 8")}
  };

  Menu *rootMenu = newMenu();
  if (!rootMenu) goto noMenu;

  {
    SUBMENU(optionsSubmenu, rootMenu, strtext("Menu Options"));

    {
      NAME(strtext("Save on Exit"));
      ITEM(newBooleanMenuItem(optionsSubmenu, &prefs.saveOnExit, &itemName));
    }

    {
      NAME(strtext("Show Submenu Sizes"));
      ITEM(newBooleanMenuItem(optionsSubmenu, &prefs.showSubmenuSizes, &itemName));
    }

    {
      NAME(strtext("Show Advanced Submenus"));
      ITEM(newBooleanMenuItem(optionsSubmenu, &prefs.showAdvancedSubmenus, &itemName));
    }

    {
      NAME(strtext("Show All Items"));
      ITEM(newBooleanMenuItem(optionsSubmenu, &prefs.showAllItems, &itemName));
    }
  }

  {
    SUBMENU(presentationSubmenu, rootMenu, strtext("Braille Presentation"));

    {
      static const MenuString strings[] = {
        {.label=strtext("8-Dot Computer Braille")},
        {.label=strtext("Contracted Braille")},
        {.label=strtext("6-Dot Computer Braille")}
      };

      NAME(strtext("Text Style"));
      ITEM(newEnumeratedMenuItem(presentationSubmenu, &prefs.textStyle, &itemName, strings));
    }

#ifdef ENABLE_CONTRACTED_BRAILLE
    {
      NAME(strtext("Expand Current Word"));
      ITEM(newBooleanMenuItem(presentationSubmenu, &prefs.expandCurrentWord, &itemName));
      TEST(ContractedBraille);
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("No Capitalization")},
        {.label=strtext("Use Capital Sign")},
        {.label=strtext("Superimpose Dot 7")}
      };

      NAME(strtext("Capitalization Mode"));
      ITEM(newEnumeratedMenuItem(presentationSubmenu, &prefs.capitalizationMode, &itemName, strings));
      TEST(ContractedBraille);
    }
#endif /* ENABLE_CONTRACTED_BRAILLE */

    {
      static const MenuString strings[] = {
        {.label=strtext("Minimum")},
        {.label=strtext("Low")},
        {.label=strtext("Medium")},
        {.label=strtext("High")},
        {.label=strtext("Maximum")}
      };

      NAME(strtext("Braille Firmness"));
      ITEM(newEnumeratedMenuItem(presentationSubmenu, &prefs.brailleFirmness, &itemName, strings));
      TEST(BrailleFirmness);
      CHANGED(BrailleFirmness);
    }
  }

  {
    SUBMENU(indicatorsSubmenu, rootMenu, strtext("Text Indicators"));

    {
      NAME(strtext("Show Cursor"));
      ITEM(newBooleanMenuItem(indicatorsSubmenu, &prefs.showCursor, &itemName));
    }

    {
      NAME(strtext("Cursor Style"));
      ITEM(newEnumeratedMenuItem(indicatorsSubmenu, &prefs.cursorStyle, &itemName, cursorStyles));
      TEST(ShowCursor);
    }

    {
      NAME(strtext("Blinking Cursor"));
      ITEM(newBooleanMenuItem(indicatorsSubmenu, &prefs.blinkingCursor, &itemName));
      TEST(ShowCursor);
    }

    {
      NAME(strtext("Cursor Visible Time"));
      ITEM(newTimeMenuItem(indicatorsSubmenu, &prefs.cursorVisibleTime, &itemName));
      TEST(BlinkingCursor);
    }

    {
      NAME(strtext("Cursor Invisible Time"));
      ITEM(newTimeMenuItem(indicatorsSubmenu, &prefs.cursorInvisibleTime, &itemName));
      TEST(BlinkingCursor);
    }

    {
      NAME(strtext("Show Attributes"));
      ITEM(newBooleanMenuItem(indicatorsSubmenu, &prefs.showAttributes, &itemName));
    }

    {
      NAME(strtext("Blinking Attributes"));
      ITEM(newBooleanMenuItem(indicatorsSubmenu, &prefs.blinkingAttributes, &itemName));
      TEST(ShowAttributes);
    }

    {
      NAME(strtext("Attributes Visible Time"));
      ITEM(newTimeMenuItem(indicatorsSubmenu, &prefs.attributesVisibleTime, &itemName));
      TEST(BlinkingAttributes);
    }

    {
      NAME(strtext("Attributes Invisible Time"));
      ITEM(newTimeMenuItem(indicatorsSubmenu, &prefs.attributesInvisibleTime, &itemName));
      TEST(BlinkingAttributes);
    }

    {
      NAME(strtext("Blinking Capitals"));
      ITEM(newBooleanMenuItem(indicatorsSubmenu, &prefs.blinkingCapitals, &itemName));
    }

    {
      NAME(strtext("Capitals Visible Time"));
      ITEM(newTimeMenuItem(indicatorsSubmenu, &prefs.capitalsVisibleTime, &itemName));
      TEST(BlinkingCapitals);
    }

    {
      NAME(strtext("Capitals Invisible Time"));
      ITEM(newTimeMenuItem(indicatorsSubmenu, &prefs.capitalsInvisibleTime, &itemName));
      TEST(BlinkingCapitals);
    }
  }

  {
    SUBMENU(navigationSubmenu, rootMenu, strtext("Navigation Options"));

    {
      NAME(strtext("Skip Identical Lines"));
      ITEM(newBooleanMenuItem(navigationSubmenu, &prefs.skipIdenticalLines, &itemName));
    }

    {
      NAME(strtext("Skip Blank Windows"));
      ITEM(newBooleanMenuItem(navigationSubmenu, &prefs.skipBlankWindows, &itemName));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("All")},
        {.label=strtext("End of Line")},
        {.label=strtext("Rest of Line")}
      };

      NAME(strtext("Which Blank Windows"));
      ITEM(newEnumeratedMenuItem(navigationSubmenu, &prefs.skipBlankWindowsMode, &itemName, strings));
      TEST(SkipBlankWindows);
    }

    {
      NAME(strtext("Sliding Window"));
      ITEM(newBooleanMenuItem(navigationSubmenu, &prefs.slidingWindow, &itemName));
    }

    {
      NAME(strtext("Eager Sliding Window"));
      ITEM(newBooleanMenuItem(navigationSubmenu, &prefs.eagerSlidingWindow, &itemName));
      TEST(SlidingWindow);
    }

    {
      NAME(strtext("Window Overlap"));
      ITEM(newNumericMenuItem(navigationSubmenu, &prefs.windowOverlap, &itemName, 0, 20, 1));
      CHANGED(WindowOverlap);
    }

#ifdef HAVE_LIBGPM
    {
      NAME(strtext("Window Follows Pointer"));
      ITEM(newBooleanMenuItem(navigationSubmenu, &prefs.windowFollowsPointer, &itemName));
    }
#endif /* HAVE_LIBGPM */

    {
      NAME(strtext("Highlight Window"));
      ITEM(newBooleanMenuItem(navigationSubmenu, &prefs.highlightWindow, &itemName));
    }
  }

  {
    SUBMENU(controlsSubmenu, rootMenu, strtext("Input Options"));

    {
      NAME(strtext("Autorepeat"));
      ITEM(newBooleanMenuItem(controlsSubmenu, &prefs.autorepeat, &itemName));
      CHANGED(Autorepeat);
    }

    {
      NAME(strtext("Autorepeat Panning"));
      ITEM(newBooleanMenuItem(controlsSubmenu, &prefs.autorepeatPanning, &itemName));
      TEST(Autorepeat);
    }

    {
      NAME(strtext("Autorepeat Delay"));
      ITEM(newTimeMenuItem(controlsSubmenu, &prefs.autorepeatDelay, &itemName));
      TEST(Autorepeat);
    }

    {
      NAME(strtext("Autorepeat Interval"));
      ITEM(newTimeMenuItem(controlsSubmenu, &prefs.autorepeatInterval, &itemName));
      TEST(Autorepeat);
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("Minimum")},
        {.label=strtext("Low")},
        {.label=strtext("Medium")},
        {.label=strtext("High")},
        {.label=strtext("Maximum")}
      };

      NAME(strtext("Braille Sensitivity"));
      ITEM(newEnumeratedMenuItem(controlsSubmenu, &prefs.brailleSensitivity, &itemName, strings));
      TEST(BrailleSensitivity);
      CHANGED(BrailleSensitivity);
    }
  }

  {
    SUBMENU(alertsSubmenu, rootMenu, strtext("Event Alerts"));

    {
      NAME(strtext("Alert Tunes"));
      ITEM(newBooleanMenuItem(alertsSubmenu, &prefs.alertTunes, &itemName));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("Beeper"), .comment=strtext("console tone generator")},
        {.label=strtext("PCM"), .comment=strtext("soundcard digital audio")},
        {.label=strtext("MIDI"), .comment=strtext("Musical Instrument Digital Interface")},
        {.label=strtext("FM"), .comment=strtext("soundcard synthesizer")}
      };

      NAME(strtext("Tune Device"));
      ITEM(newEnumeratedMenuItem(alertsSubmenu, &prefs.tuneDevice, &itemName, strings));
      TEST(Tunes);
      CHANGED(TuneDevice);
    }

#ifdef ENABLE_PCM_SUPPORT
    {
      NAME(strtext("PCM Volume"));
      ITEM(newVolumeMenuItem(alertsSubmenu, &prefs.pcmVolume, &itemName));
      TEST(TunesPcm);
    }
#endif /* ENABLE_PCM_SUPPORT */

#ifdef ENABLE_MIDI_SUPPORT
    {
      NAME(strtext("MIDI Volume"));
      ITEM(newVolumeMenuItem(alertsSubmenu, &prefs.midiVolume, &itemName));
      TEST(TunesMidi);
    }

    {
      const MenuString *strings = makeMidiInstrumentMenuStrings();
      if (!strings) goto noItem;

      {
        NAME(strtext("MIDI Instrument"));
        ITEM(newStringsMenuItem(alertsSubmenu, &prefs.midiInstrument, &itemName, strings, midiInstrumentCount));
        TEST(TunesMidi);
      }
    }
#endif /* ENABLE_MIDI_SUPPORT */

#ifdef ENABLE_FM_SUPPORT
    {
      NAME(strtext("FM Volume"));
      ITEM(newVolumeMenuItem(alertsSubmenu, &prefs.fmVolume, &itemName));
      TEST(TunesFm);
    }
#endif /* ENABLE_FM_SUPPORT */

    {
      NAME(strtext("Alert Dots"));
      ITEM(newBooleanMenuItem(alertsSubmenu, &prefs.alertDots, &itemName));
    }

    {
      NAME(strtext("Alert Messages"));
      ITEM(newBooleanMenuItem(alertsSubmenu, &prefs.alertMessages, &itemName));
    }
  }

#ifdef ENABLE_SPEECH_SUPPORT
  {
    SUBMENU(speechSubmenu, rootMenu, strtext("Speech Options"));

    {
      NAME(strtext("Speech Volume"));
      ITEM(newNumericMenuItem(speechSubmenu, &prefs.speechVolume, &itemName, 0, SPK_VOLUME_MAXIMUM, 1));
      TEST(SpeechVolume);
      CHANGED(SpeechVolume);
    }

    {
      NAME(strtext("Speech Rate"));
      ITEM(newNumericMenuItem(speechSubmenu, &prefs.speechRate, &itemName, 0, SPK_RATE_MAXIMUM, 1));
      TEST(SpeechRate);
      CHANGED(SpeechRate);
    }

    {
      NAME(strtext("Speech Pitch"));
      ITEM(newNumericMenuItem(speechSubmenu, &prefs.speechPitch, &itemName, 0, SPK_PITCH_MAXIMUM, 1));
      TEST(SpeechPitch);
      CHANGED(SpeechPitch);
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("None")},
        {.label=strtext("Some")},
        {.label=strtext("All")}
      };

      NAME(strtext("Speech Punctuation"));
      ITEM(newEnumeratedMenuItem(speechSubmenu, &prefs.speechPunctuation, &itemName, strings));
      TEST(SpeechPunctuation);
      CHANGED(SpeechPunctuation);
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("None")},
        // "cap" here, used during speech output, is short for "capital".
        // It is spoken just before an uppercase letter, e.g. "cap A".
        {.label=strtext("Say Cap")},
        {.label=strtext("Raise Pitch")}
      };

      NAME(strtext("Uppercase Indicator"));
      ITEM(newEnumeratedMenuItem(speechSubmenu, &prefs.uppercaseIndicator, &itemName, strings));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("None")},
        {.label=strtext("Say Space")},
      };

      NAME(strtext("Whitespace Indicator"));
      ITEM(newEnumeratedMenuItem(speechSubmenu, &prefs.whitespaceIndicator, &itemName, strings));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("Immediate")},
        {.label=strtext("Enqueue")}
      };

      NAME(strtext("Say Line Mode"));
      ITEM(newEnumeratedMenuItem(speechSubmenu, &prefs.sayLineMode, &itemName, strings));
    }

    {
      NAME(strtext("Autospeak"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeak, &itemName));
    }

    {
      NAME(strtext("Speak Selected Line"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeakSelectedLine, &itemName));
      TEST(Autospeak);
    }

    {
      NAME(strtext("Speak Selected Character"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeakSelectedCharacter, &itemName));
      TEST(Autospeak);
    }

    {
      NAME(strtext("Speak Inserted Characters"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeakInsertedCharacters, &itemName));
      TEST(Autospeak);
    }

    {
      NAME(strtext("Speak Deleted Characters"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeakDeletedCharacters, &itemName));
      TEST(Autospeak);
    }

    {
      NAME(strtext("Speak Replaced Characters"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeakReplacedCharacters, &itemName));
      TEST(Autospeak);
    }

    {
      NAME(strtext("Speak Completed Words"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.autospeakCompletedWords, &itemName));
      TEST(Autospeak);
    }

    {
      NAME(strtext("Show Speech Cursor"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.showSpeechCursor, &itemName));
    }

    {
      NAME(strtext("Speech Cursor Style"));
      ITEM(newEnumeratedMenuItem(speechSubmenu, &prefs.speechCursorStyle, &itemName, cursorStyles));
      TEST(ShowSpeechCursor);
    }

    {
      NAME(strtext("Blinking Speech Cursor"));
      ITEM(newBooleanMenuItem(speechSubmenu, &prefs.blinkingSpeechCursor, &itemName));
      TEST(ShowSpeechCursor);
    }

    {
      NAME(strtext("Speech Cursor Visible Time"));
      ITEM(newTimeMenuItem(speechSubmenu, &prefs.speechCursorVisibleTime, &itemName));
      TEST(BlinkingSpeechCursor);
    }

    {
      NAME(strtext("Speech Cursor Invisible Time"));
      ITEM(newTimeMenuItem(speechSubmenu, &prefs.speechCursorInvisibleTime, &itemName));
      TEST(BlinkingSpeechCursor);
    }
  }
#endif /* ENABLE_SPEECH_SUPPORT */

  {
    SUBMENU(timeSubmenu, rootMenu, strtext("Time Presentation"));

    {
      static const MenuString strings[] = {
        {.label=strtext("24 Hour")},
        {.label=strtext("12 Hour")}
      };

      NAME(strtext("Time Format"));
      ITEM(newEnumeratedMenuItem(timeSubmenu, &prefs.timeFormat, &itemName, strings));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("Colon"), ":"},
        {.label=strtext("Dot"), "."},
      };

      NAME(strtext("Time Separator"));
      ITEM(newEnumeratedMenuItem(timeSubmenu, &prefs.timeSeparator, &itemName, strings));
    }

    {
      NAME(strtext("Show Seconds"));
      ITEM(newBooleanMenuItem(timeSubmenu, &prefs.showSeconds, &itemName));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("None")},
        {.label=strtext("Before Time")},
        {.label=strtext("After Time")}
      };

      NAME(strtext("Date Position"));
      ITEM(newEnumeratedMenuItem(timeSubmenu, &prefs.datePosition, &itemName, strings));
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("Year Month Day")},
        {.label=strtext("Month Day Year")},
        {.label=strtext("Day Month Year")},
      };

      NAME(strtext("Date Format"));
      ITEM(newEnumeratedMenuItem(timeSubmenu, &prefs.dateFormat, &itemName, strings));
      TEST(ShowDate);
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("Dash"), "-"},
        {.label=strtext("Slash"), "/"},
        {.label=strtext("Dot"), "."}
      };

      NAME(strtext("Date Separator"));
      ITEM(newEnumeratedMenuItem(timeSubmenu, &prefs.dateSeparator, &itemName, strings));
      TEST(ShowDate);
    }
  }

  {
    SUBMENU(statusSubmenu, rootMenu, strtext("Status Cells"));

    {
      static const MenuString strings[] = {
        {.label=strtext("None")},
        {.label=strtext("Left")},
        {.label=strtext("Right")}
      };

      NAME(strtext("Status Position"));
      ITEM(newEnumeratedMenuItem(statusSubmenu, &prefs.statusPosition, &itemName, strings));
      TEST(StatusPosition);
      CHANGED(StatusPosition);
    }

    {
      NAME(strtext("Status Count"));
      ITEM(newNumericMenuItem(statusSubmenu, &prefs.statusCount, &itemName, 0, MAX((int)brl.textColumns/2-1, 0), 1));
      TEST(StatusCount);
      CHANGED(StatusCount);
    }

    {
      static const MenuString strings[] = {
        {.label=strtext("None")},
        {.label=strtext("Space")},
        {.label=strtext("Block")},
        {.label=strtext("Status Side")},
        {.label=strtext("Text Side")}
      };

      NAME(strtext("Status Separator"));
      ITEM(newEnumeratedMenuItem(statusSubmenu, &prefs.statusSeparator, &itemName, strings));
      TEST(StatusSeparator);
      CHANGED(StatusSeparator);
    }

    {
#define STATUS_FIELD_ITEM(number) { ITEM(newStatusFieldMenuItem(statusSubmenu, number, testStatusField##number, changedStatusField##number)); }
      STATUS_FIELD_ITEM(1);
      STATUS_FIELD_ITEM(2);
      STATUS_FIELD_ITEM(3);
      STATUS_FIELD_ITEM(4);
      STATUS_FIELD_ITEM(5);
      STATUS_FIELD_ITEM(6);
      STATUS_FIELD_ITEM(7);
      STATUS_FIELD_ITEM(8);
      STATUS_FIELD_ITEM(9);
#undef STATUS_FIELD_ITEM
    }
  }

  {
    SUBMENU(tablesSubmenu, rootMenu, strtext("Braille Tables"));

    {
      NAME(strtext("Text Table"));
      ITEM(newFilesMenuItem(tablesSubmenu, &itemName, opt_tablesDirectory, TEXT_TABLE_EXTENSION, opt_textTable, 0));
      CHANGED(TextTable);
    }

    {
      NAME(strtext("Attributes Table"));
      ITEM(newFilesMenuItem(tablesSubmenu, &itemName, opt_tablesDirectory, ATTRIBUTES_TABLE_EXTENSION, opt_attributesTable, 0));
      CHANGED(AttributesTable);
    }

#ifdef ENABLE_CONTRACTED_BRAILLE
    {
      NAME(strtext("Contraction Table"));
      ITEM(newFilesMenuItem(tablesSubmenu, &itemName, opt_tablesDirectory, CONTRACTION_TABLE_EXTENSION, opt_contractionTable, 1));
      CHANGED(ContractionTable);
    }
#endif /* ENABLE_CONTRACTED_BRAILLE */
  }

  {
    static const MenuString logLevels[] = {
      {.label=strtext("Emergency")},
      {.label=strtext("Alert")},
      {.label=strtext("Critical")},
      {.label=strtext("Error")},
      {.label=strtext("Warning")},
      {.label=strtext("Notice")},
      {.label=strtext("Information")},
      {.label=strtext("Debug")}
    };

    SUBMENU(internalSubmenu, rootMenu, strtext("Internal Parameters"));
    setAdvancedSubmenu(internalSubmenu);

    {
      NAME(strtext("System Log Level"));
      ITEM(newEnumeratedMenuItem(internalSubmenu, &systemLogLevel, &itemName, logLevels));
    }

    {
      NAME(strtext("Standard Error Log Level"));
      ITEM(newEnumeratedMenuItem(internalSubmenu, &stderrLogLevel, &itemName, logLevels));
    }

    {
      NAME(strtext("Category Log Level"));
      ITEM(newEnumeratedMenuItem(internalSubmenu, &categoryLogLevel, &itemName, logLevels));
    }

    {
      NAME(strtext("Log Generic Input"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(GENERIC_INPUT), &itemName));
    }

    {
      NAME(strtext("Log Input Packets"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(INPUT_PACKETS), &itemName));
    }

    {
      NAME(strtext("Log Output Packets"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(OUTPUT_PACKETS), &itemName));
    }

    {
      NAME(strtext("Log Braille Key Events"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(BRAILLE_KEY_EVENTS), &itemName));
      TEST(BrailleKeyTable);
    }

    {
      NAME(strtext("Log Keyboard Key Events"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(KEYBOARD_KEY_EVENTS), &itemName));
      TEST(KeyboardKeyTable);
    }

    {
      NAME(strtext("Log Cursor Tracking"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(CURSOR_TRACKING), &itemName));
    }

    {
      NAME(strtext("Log Cursor Routing"));
      ITEM(newBooleanMenuItem(internalSubmenu, &LOG_CATEGORY_FLAG(CURSOR_ROUTING), &itemName));
    }
  }

  return rootMenu;

noItem:
  deallocateMenu(rootMenu);
noMenu:
  return NULL;
}

Menu *
getPreferencesMenu (void) {
  static Menu *menu = NULL;
  if (!menu) menu = makePreferencesMenu();
  return menu;
}
