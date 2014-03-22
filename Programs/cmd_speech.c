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

#include <stdio.h>

#include "cmd_speech.h"
#include "prefs.h"
#include "tunes.h"
#include "brl_cmds.h"
#include "spk.h"
#include "scr.h"
#include "brltty.h"

#ifdef ENABLE_SPEECH_SUPPORT
static void
sayScreenRegion (int left, int top, int width, int height, int track, SayMode mode) {
  size_t count = width * height;
  ScreenCharacter characters[count];

  if (mode == sayImmediate) muteSpeech(__func__);
  readScreen(left, top, width, height, characters);
  spk.track.isActive = track;
  spk.track.screenNumber = scr.number;
  spk.track.firstLine = top;
  spk.track.speechLocation = SPK_LOC_NONE;
  sayScreenCharacters(characters, count, 0);
}

static void
sayScreenLines (int line, int count, int track, SayMode mode) {
  sayScreenRegion(0, line, scr.cols, count, track, mode);
}

static void
speakDone (const ScreenCharacter *line, int column, int count, int spell) {
  ScreenCharacter internalBuffer[count];

  if (line) {
    line = &line[column];
  } else {
    readScreen(column, ses->spky, count, 1, internalBuffer);
    line = internalBuffer;
  }

  speakCharacters(line, count, spell);
  placeWindowHorizontally(ses->spkx);
  slideWindowVertically(ses->spky);
}

static void
speakCurrentCharacter (void) {
  speakDone(NULL, ses->spkx, 1, 0);
}

static void
speakCurrentLine (void) {
  speakDone(NULL, 0, scr.cols, 0);
}

int
handleSpeechCommand (int command, void *data) {
  switch (command & BRL_MSK_CMD) {
    case BRL_CMD_RESTARTSPEECH:
      restartSpeechDriver();
      break;
    case BRL_CMD_SPKHOME:
      if (scr.number == spk.track.screenNumber) {
        trackSpeech();
      } else {
        playTune(&tune_command_rejected);
      }
      break;
    case BRL_CMD_AUTOSPEAK:
      toggleFeatureSetting(&prefs.autospeak, command);
      break;

    case BRL_CMD_ASPK_SEL_LINE:
      toggleFeatureSetting(&prefs.autospeakSelectedLine, command);
      break;

    case BRL_CMD_ASPK_SEL_CHAR:
      toggleFeatureSetting(&prefs.autospeakSelectedCharacter, command);
      break;

    case BRL_CMD_ASPK_INS_CHARS:
      toggleFeatureSetting(&prefs.autospeakInsertedCharacters, command);
      break;

    case BRL_CMD_ASPK_DEL_CHARS:
      toggleFeatureSetting(&prefs.autospeakDeletedCharacters, command);
      break;

    case BRL_CMD_ASPK_REP_CHARS:
      toggleFeatureSetting(&prefs.autospeakReplacedCharacters, command);
      break;

    case BRL_CMD_ASPK_CMP_WORDS:
      toggleFeatureSetting(&prefs.autospeakCompletedWords, command);
      break;

    case BRL_CMD_MUTE:
      muteSpeech("command");
      break;

    case BRL_CMD_SAY_LINE:
      sayScreenLines(ses->winy, 1, 0, prefs.sayLineMode);
      break;
    case BRL_CMD_SAY_ABOVE:
      sayScreenLines(0, ses->winy+1, 1, sayImmediate);
      break;
    case BRL_CMD_SAY_BELOW:
      sayScreenLines(ses->winy, scr.rows-ses->winy, 1, sayImmediate);
      break;

    case BRL_CMD_SAY_SLOWER:
      if (!canSetSpeechRate()) {
        playTune(&tune_command_rejected);
      } else if (prefs.speechRate > 0) {
        setSpeechRate(--prefs.speechRate, 1);
      } else {
        playTune(&tune_no_change);
      }
      break;

    case BRL_CMD_SAY_FASTER:
      if (!canSetSpeechRate()) {
        playTune(&tune_command_rejected);
      } else if (prefs.speechRate < SPK_RATE_MAXIMUM) {
        setSpeechRate(++prefs.speechRate, 1);
      } else {
        playTune(&tune_no_change);
      }
      break;

    case BRL_CMD_SAY_SOFTER:
      if (!canSetSpeechVolume()) {
        playTune(&tune_command_rejected);
      } else if (prefs.speechVolume > 0) {
        setSpeechVolume(--prefs.speechVolume, 1);
      } else {
        playTune(&tune_no_change);
      }
      break;

    case BRL_CMD_SAY_LOUDER:
      if (!canSetSpeechVolume()) {
        playTune(&tune_command_rejected);
      } else if (prefs.speechVolume < SPK_VOLUME_MAXIMUM) {
        setSpeechVolume(++prefs.speechVolume, 1);
      } else {
        playTune(&tune_no_change);
      }
      break;

    case BRL_CMD_SPEAK_CURR_CHAR:
      speakCurrentCharacter();
      break;

    case BRL_CMD_SPEAK_PREV_CHAR:
      if (ses->spkx > 0) {
        ses->spkx -= 1;
        speakCurrentCharacter();
      } else if (ses->spky > 0) {
        ses->spky -= 1;
        ses->spkx = scr.cols - 1;
        playTune(&tune_wrap_up);
        speakCurrentCharacter();
      } else {
        playTune(&tune_bounce);
      }
      break;

    case BRL_CMD_SPEAK_NEXT_CHAR:
      if (ses->spkx < (scr.cols - 1)) {
        ses->spkx += 1;
        speakCurrentCharacter();
      } else if (ses->spky < (scr.rows - 1)) {
        ses->spky += 1;
        ses->spkx = 0;
        playTune(&tune_wrap_down);
        speakCurrentCharacter();
      } else {
        playTune(&tune_bounce);
      }
      break;

    case BRL_CMD_SPEAK_FRST_CHAR: {
      ScreenCharacter characters[scr.cols];
      int column;

      readScreen(0, ses->spky, scr.cols, 1, characters);
      if ((column = findFirstNonSpaceCharacter(characters, scr.cols)) >= 0) {
        ses->spkx = column;
        speakDone(characters, column, 1, 0);
      } else {
        playTune(&tune_command_rejected);
      }

      break;
    }

    case BRL_CMD_SPEAK_LAST_CHAR: {
      ScreenCharacter characters[scr.cols];
      int column;

      readScreen(0, ses->spky, scr.cols, 1, characters);
      if ((column = findLastNonSpaceCharacter(characters, scr.cols)) >= 0) {
        ses->spkx = column;
        speakDone(characters, column, 1, 0);
      } else {
        playTune(&tune_command_rejected);
      }

      break;
    }

    {
      int direction;
      int spell;

    case BRL_CMD_SPEAK_PREV_WORD:
      direction = -1;
      spell = 0;
      goto speakWord;

    case BRL_CMD_SPEAK_NEXT_WORD:
      direction = 1;
      spell = 0;
      goto speakWord;

    case BRL_CMD_SPEAK_CURR_WORD:
      direction = 0;
      spell = 0;
      goto speakWord;

    case BRL_CMD_SPELL_CURR_WORD:
      direction = 0;
      spell = 1;
      goto speakWord;

    speakWord:
      {
        int row = ses->spky;
        int column = ses->spkx;

        ScreenCharacter characters[scr.cols];
        ScreenCharacterType type;
        int onCurrentWord;

        int from = column;
        int to = from + 1;

      findWord:
        readScreen(0, row, scr.cols, 1, characters);
        type = (row == ses->spky)? getScreenCharacterType(&characters[column]): SCT_SPACE;
        onCurrentWord = type != SCT_SPACE;

        if (direction < 0) {
          while (1) {
            if (column == 0) {
              if ((type != SCT_SPACE) && !onCurrentWord) {
                ses->spkx = from = column;
                ses->spky = row;
                break;
              }

              if (row == 0) goto noWord;
              if (row-- == ses->spky) playTune(&tune_wrap_up);
              column = scr.cols;
              goto findWord;
            }

            {
              ScreenCharacterType newType = getScreenCharacterType(&characters[--column]);

              if (newType != type) {
                if (onCurrentWord) {
                  onCurrentWord = 0;
                } else if (type != SCT_SPACE) {
                  ses->spkx = from = column + 1;
                  ses->spky = row;
                  break;
                }

                if (newType != SCT_SPACE) to = column + 1;
                type = newType;
              }
            }
          }
        } else if (direction > 0) {
          while (1) {
            if (++column == scr.cols) {
              if ((type != SCT_SPACE) && !onCurrentWord) {
                to = column;
                ses->spkx = from;
                ses->spky = row;
                break;
              }

              if (row == (scr.rows - 1)) goto noWord;
              if (row++ == ses->spky) playTune(&tune_wrap_down);
              column = -1;
              goto findWord;
            }

            {
              ScreenCharacterType newType = getScreenCharacterType(&characters[column]);

              if (newType != type) {
                if (onCurrentWord) {
                  onCurrentWord = 0;
                } else if (type != SCT_SPACE) {
                  to = column;
                  ses->spkx = from;
                  ses->spky = row;
                  break;
                }

                if (newType != SCT_SPACE) from = column;
                type = newType;
              }
            }
          }
        } else if (type != SCT_SPACE) {
          while (from > 0) {
            if (getScreenCharacterType(&characters[--from]) != type) {
              from += 1;
              break;
            }
          }

          while (to < scr.cols) {
            if (getScreenCharacterType(&characters[to]) != type) break;
            to += 1;
          }
        }

        speakDone(characters, from, to-from, spell);
        break;
      }

    noWord:
      playTune(&tune_bounce);
      break;
    }

    case BRL_CMD_SPEAK_CURR_LINE:
      speakCurrentLine();
      break;

    {
      int increment;
      int limit;

    case BRL_CMD_SPEAK_PREV_LINE:
      increment = -1;
      limit = 0;
      goto speakLine;

    case BRL_CMD_SPEAK_NEXT_LINE:
      increment = 1;
      limit = scr.rows - 1;
      goto speakLine;

    speakLine:
      if (ses->spky == limit) {
        playTune(&tune_bounce);
      } else {
        if (prefs.skipIdenticalLines) {
          ScreenCharacter original[scr.cols];
          ScreenCharacter current[scr.cols];
          int count = 0;

          readScreen(0, ses->spky, scr.cols, 1, original);

          do {
            readScreen(0, ses->spky+=increment, scr.cols, 1, current);
            if (!isSameRow(original, current, scr.cols, isSameText)) break;

            if (!count) {
              playTune(&tune_skip_first);
            } else if (count < 4) {
              playTune(&tune_skip);
            } else if (!(count % 4)) {
              playTune(&tune_skip_more);
            }

            count += 1;
          } while (ses->spky != limit);
        } else {
          ses->spky += increment;
        }

        speakCurrentLine();
      }

      break;
    }

    case BRL_CMD_SPEAK_FRST_LINE: {
      ScreenCharacter characters[scr.cols];
      int row = 0;

      while (row < scr.rows) {
        readScreen(0, row, scr.cols, 1, characters);
        if (!isAllSpaceCharacters(characters, scr.cols)) break;
        row += 1;
      }

      if (row < scr.rows) {
        ses->spky = row;
        ses->spkx = 0;
        speakCurrentLine();
      } else {
        playTune(&tune_command_rejected);
      }

      break;
    }

    case BRL_CMD_SPEAK_LAST_LINE: {
      ScreenCharacter characters[scr.cols];
      int row = scr.rows - 1;

      while (row >= 0) {
        readScreen(0, row, scr.cols, 1, characters);
        if (!isAllSpaceCharacters(characters, scr.cols)) break;
        row -= 1;
      }

      if (row >= 0) {
        ses->spky = row;
        ses->spkx = 0;
        speakCurrentLine();
      } else {
        playTune(&tune_command_rejected);
      }

      break;
    }

    case BRL_CMD_DESC_CURR_CHAR: {
      char description[0X50];
      formatCharacterDescription(description, sizeof(description), ses->spkx, ses->spky);
      sayString(description, 1);
      break;
    }

    case BRL_CMD_ROUTE_CURR_LOCN:
      if (routeCursor(ses->spkx, ses->spky, scr.number)) {
        playTune(&tune_routing_started);
      } else {
        playTune(&tune_command_rejected);
      }
      break;

    case BRL_CMD_SPEAK_CURR_LOCN: {
      char buffer[0X50];
      snprintf(buffer, sizeof(buffer), "%s %d, %s %d",
               gettext("line"), ses->spky+1,
               gettext("column"), ses->spkx+1);
      sayString(buffer, 1);
      break;
    }

    case BRL_CMD_SHOW_CURR_LOCN:
      toggleFeatureSetting(&prefs.showSpeechCursor, command);
      break;

    default:
      return 0;
  }

  return 1;
}
#endif /* ENABLE_SPEECH_SUPPORT */
