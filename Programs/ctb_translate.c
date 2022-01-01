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

#include "prologue.h"

#include <string.h>

#include "log.h"
#include "lock.h"
#include "ctb_translate.h"
#include "ttb.h"
#include "unicode.h"
#include "prefs.h"

static const unsigned char internalContractionTableBytes[] = {
#include "ctb.auto.h"
};

const unsigned char *
getInternalContractionTableBytes (void) {
  return internalContractionTableBytes;
}

ContractionTable *contractionTable = NULL;

static LockDescriptor *
getContractionTableLock (void) {
  static LockDescriptor *lock = NULL;
  return getLockDescriptor(&lock, "contraction-table");
}

void
lockContractionTable (void) {
  obtainExclusiveLock(getContractionTableLock());
}

void
unlockContractionTable (void) {
  releaseLock(getContractionTableLock());
}

const CharacterEntry *
findCharacterEntry (BrailleContractionData *bcd, wchar_t character, unsigned int *position) {
  unsigned int from = 0;
  unsigned int to = bcd->table->characters.count;

  while (from < to) {
    unsigned int current = (from + to) / 2;
    const CharacterEntry *entry = &bcd->table->characters.array[current];

    if (entry->value < character) {
      from = current + 1;
    } else if (entry->value > character) {
      to = current;
    } else {
      if (position) *position = current;
      return entry;
    }
  }

  if (position) *position = from;
  return NULL;
}

static const CharacterEntry *
addCharacterEntry (BrailleContractionData *bcd, wchar_t character, unsigned int position) {
  ContractionTable *table = bcd->table;

  if (table->characters.count == table->characters.size) {
    int newSize = table->characters.size;
    newSize = newSize? newSize<<1: 0X80;
    CharacterEntry *newArray = realloc(table->characters.array, ARRAY_SIZE(newArray, newSize));

    if (!newArray) {
      logMallocError();
      return NULL;
    }

    table->characters.array = newArray;
    table->characters.size = newSize;
  }

  memmove(
    &table->characters.array[position+1],
    &table->characters.array[position],
    ((table->characters.count++ - position) * sizeof(*table->characters.array))
  );

  CharacterEntry *entry = &bcd->table->characters.array[position];
  memset(entry, 0, sizeof(*entry));
  entry->value = entry->uppercase = entry->lowercase = character;

  if (iswspace(character)) {
    entry->attributes |= CTC_Space;
  } else if (iswalpha(character)) {
    entry->attributes |= CTC_Letter;

    if (iswupper(character)) {
      entry->attributes |= CTC_UpperCase;
      entry->lowercase = towlower(character);
    }

    if (iswlower(character)) {
      entry->attributes |= CTC_LowerCase;
      entry->uppercase = towupper(character);
    }
  } else if (iswdigit(character)) {
    entry->attributes |= CTC_Digit;
  } else if (iswpunct(character)) {
    entry->attributes |= CTC_Punctuation;
  }

  bcd->table->translationMethods->finishCharacterEntry(bcd, entry);
  return entry;
}

const CharacterEntry *
getCharacterEntry (BrailleContractionData *bcd, wchar_t character) {
  unsigned int position;
  const CharacterEntry *entry = findCharacterEntry(bcd, character, &position);
  if (entry) return entry;
  return addCharacterEntry(bcd, character, position);
}

static inline int
makeCachedCursorOffset (BrailleContractionData *bcd) {
  return bcd->input.cursor? (bcd->input.cursor - bcd->input.begin): CTB_NO_CURSOR;
}

static int
checkCache (BrailleContractionData *bcd) {
  if (!bcd->table->cache.input.characters) return 0;
  if (!bcd->table->cache.output.cells) return 0;
  if (bcd->input.offsets && !bcd->table->cache.offsets.count) return 0;
  if (bcd->table->cache.output.maximum != getOutputCount(bcd)) return 0;
  if (bcd->table->cache.cursorOffset != makeCachedCursorOffset(bcd)) return 0;
  if (bcd->table->cache.expandCurrentWord != prefs.expandCurrentWord) return 0;
  if (bcd->table->cache.capitalizationMode != prefs.capitalizationMode) return 0;

  {
    unsigned int count = getInputCount(bcd);
    if (bcd->table->cache.input.count != count) return 0;
    if (wmemcmp(bcd->input.begin, bcd->table->cache.input.characters, count) != 0) return 0;
  }

  return 1;
}

static void
updateCache (BrailleContractionData *bcd) {
  {
    unsigned int count = getInputCount(bcd);

    if (count > bcd->table->cache.input.size) {
      unsigned int newSize = count | 0X7F;
      wchar_t *newCharacters = malloc(ARRAY_SIZE(newCharacters, newSize));

      if (!newCharacters) {
        logMallocError();
        bcd->table->cache.input.count = 0;
        goto inputDone;
      }

      if (bcd->table->cache.input.characters) free(bcd->table->cache.input.characters);
      bcd->table->cache.input.characters = newCharacters;
      bcd->table->cache.input.size = newSize;
    }

    wmemcpy(bcd->table->cache.input.characters, bcd->input.begin, count);
    bcd->table->cache.input.count = count;
    bcd->table->cache.input.consumed = getInputConsumed(bcd);
  }
inputDone:

  {
    unsigned int count = getOutputConsumed(bcd);

    if (count > bcd->table->cache.output.size) {
      unsigned int newSize = count | 0X7F;
      unsigned char *newCells = malloc(ARRAY_SIZE(newCells, newSize));

      if (!newCells) {
        logMallocError();
        bcd->table->cache.output.count = 0;
        goto outputDone;
      }

      if (bcd->table->cache.output.cells) free(bcd->table->cache.output.cells);
      bcd->table->cache.output.cells = newCells;
      bcd->table->cache.output.size = newSize;
    }

    memcpy(bcd->table->cache.output.cells, bcd->output.begin, count);
    bcd->table->cache.output.count = count;
    bcd->table->cache.output.maximum = getOutputCount(bcd);
  }
outputDone:

  if (bcd->input.offsets) {
    unsigned int count = getInputCount(bcd);

    if (count > bcd->table->cache.offsets.size) {
      unsigned int newSize = count | 0X7F;
      int *newArray = malloc(ARRAY_SIZE(newArray, newSize));

      if (!newArray) {
        logMallocError();
        bcd->table->cache.offsets.count = 0;
        goto offsetsDone;
      }

      if (bcd->table->cache.offsets.array) free(bcd->table->cache.offsets.array);
      bcd->table->cache.offsets.array = newArray;
      bcd->table->cache.offsets.size = newSize;
    }

    memcpy(bcd->table->cache.offsets.array, bcd->input.offsets, ARRAY_SIZE(bcd->input.offsets, count));
    bcd->table->cache.offsets.count = count;
  } else {
    bcd->table->cache.offsets.count = 0;
  }
offsetsDone:

  bcd->table->cache.cursorOffset = makeCachedCursorOffset(bcd);
  bcd->table->cache.expandCurrentWord = prefs.expandCurrentWord;
  bcd->table->cache.capitalizationMode = prefs.capitalizationMode;
}

void
contractText (
  ContractionTable *contractionTable,
  const wchar_t *inputBuffer, int *inputLength,
  BYTE *outputBuffer, int *outputLength,
  int *offsetsMap, const int cursorOffset
) {
  BrailleContractionData bcd = {
    .table = contractionTable,

    .input = {
      .begin = inputBuffer,
      .current = inputBuffer,
      .end = inputBuffer + *inputLength,
      .cursor = (cursorOffset == CTB_NO_CURSOR)? NULL: &inputBuffer[cursorOffset],
      .offsets = offsetsMap
    },

    .output = {
      .begin = outputBuffer,
      .end = outputBuffer + *outputLength,
      .current = outputBuffer
    }
  };

  if (checkCache(&bcd)) {
    bcd.input.current = bcd.input.begin + bcd.table->cache.input.consumed;

    if (bcd.input.offsets) {
      memcpy(bcd.input.offsets, bcd.table->cache.offsets.array,
             ARRAY_SIZE(bcd.input.offsets, bcd.table->cache.offsets.count));
    }

    bcd.output.current = bcd.output.begin + bcd.table->cache.output.count;
    memcpy(bcd.output.begin, bcd.table->cache.output.cells,
           ARRAY_SIZE(bcd.output.begin, bcd.table->cache.output.count));
  } else {
    int contracted;

    {
      size_t length = getInputCount(&bcd);
      wchar_t buffer[length];
      unsigned int map[length + 1];

      if (composeCharacters(&length, bcd.input.begin, buffer, map)) {
        const wchar_t *oldBegin = bcd.input.begin;
        const wchar_t *oldEnd = bcd.input.end;

        bcd.input.begin = buffer;
        bcd.input.current = bcd.input.begin + (bcd.input.current - oldBegin);
        bcd.input.end = bcd.input.begin + length;

        if (bcd.input.cursor) {
          ptrdiff_t offset = bcd.input.cursor - oldBegin;
          unsigned int mapIndex;

          bcd.input.cursor = NULL;

          for (mapIndex=0; mapIndex<=length; mapIndex+=1) {
            unsigned int mappedIndex = map[mapIndex];

            if (mappedIndex > offset) break;
            bcd.input.cursor = &bcd.input.begin[mappedIndex];
          }
        }

        contracted = contractionTable->translationMethods->contractText(&bcd);

        if (bcd.input.offsets) {
          size_t mapIndex = length;
          size_t offsetsIndex = oldEnd - oldBegin;

          while (mapIndex > 0) {
            unsigned int mappedIndex = map[--mapIndex];
            int offset = bcd.input.offsets[mapIndex];

            if (offset != CTB_NO_OFFSET) {
              while (--offsetsIndex > mappedIndex) bcd.input.offsets[offsetsIndex] = CTB_NO_OFFSET;
              bcd.input.offsets[offsetsIndex] = offset;
            }
          }

          while (offsetsIndex > 0) bcd.input.offsets[--offsetsIndex] = CTB_NO_OFFSET;
        }

        bcd.input.begin = oldBegin;
        bcd.input.current = bcd.input.begin + map[bcd.input.current - buffer];
        bcd.input.end = oldEnd;
      } else {
        contracted = contractionTable->translationMethods->contractText(&bcd);
      }
    }

    if (!contracted) {
      bcd.input.current = bcd.input.begin;
      bcd.output.current = bcd.output.begin;

      while ((bcd.input.current < bcd.input.end) && (bcd.output.current < bcd.output.end)) {
        setOffset(&bcd);
        *bcd.output.current++ = convertCharacterToDots(textTable, *bcd.input.current++);
      }
    }

    if (bcd.input.current < bcd.input.end) {
      const wchar_t *srcorig = bcd.input.current;
      int done = 1;

      setOffset(&bcd);
      while (1) {
        if (done && !testCurrent(&bcd, CTC_Space)) {
          done = 0;

          if (!bcd.input.cursor || (bcd.input.cursor < srcorig) || (bcd.input.cursor >= bcd.input.current)) {
            setOffset(&bcd);
            srcorig = bcd.input.current;
          }
        }

        if (++bcd.input.current == bcd.input.end) break;
        clearOffset(&bcd);
      }

      if (!done) bcd.input.current = srcorig;
    }

    updateCache(&bcd);
  }

  *inputLength = getInputConsumed(&bcd);
  *outputLength = getOutputConsumed(&bcd);
}

int
replaceContractionTable (const char *directory, const char *name) {
  ContractionTable *newTable = NULL;

  if (*name) {
    char *path = makeContractionTablePath(directory, name);

    if (path) {
      logMessage(LOG_DEBUG, "compiling contraction table: %s", path);

      if (!(newTable = compileContractionTable(path))) {
        logMessage(LOG_ERR, "%s: %s", gettext("cannot compile contraction table"), path);
      }

      free(path);
    }
  } else if (!(newTable = compileContractionTable(name))) {
    logMessage(LOG_ERR, "%s: %s", gettext("cannot access internal contraction table"), CONTRACTION_TABLE);
  }

  if (newTable) {
    ContractionTable *oldTable = contractionTable;

    lockContractionTable();
      contractionTable = newTable;
    unlockContractionTable();

    if (oldTable) destroyContractionTable(oldTable);
    return 1;
  }

  logMessage(LOG_ERR, "%s: %s", gettext("cannot load contraction table"), name);
  return 0;
}
