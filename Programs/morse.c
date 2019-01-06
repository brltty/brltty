/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2019 by The BRLTTY Developers.
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
#include "morse.h"
#include "tune.h"
#include "charset.h"

static const MorsePattern morsePatterns[] = {
  [WC_C('a')] = 0B101,
  [WC_C('b')] = 0B11110,
  [WC_C('c')] = 0B11010,
  [WC_C('d')] = 0B1110,
  [WC_C('e')] = 0B11,
  [WC_C('f')] = 0B11011,
  [WC_C('g')] = 0B1100,
  [WC_C('h')] = 0B11111,
  [WC_C('i')] = 0B111,
  [WC_C('j')] = 0B10001,
  [WC_C('k')] = 0B1010,
  [WC_C('l')] = 0B11101,
  [WC_C('m')] = 0B100,
  [WC_C('n')] = 0B110,
  [WC_C('o')] = 0B1000,
  [WC_C('p')] = 0B11001,
  [WC_C('q')] = 0B10100,
  [WC_C('r')] = 0B1101,
  [WC_C('s')] = 0B1111,
  [WC_C('t')] = 0B10,
  [WC_C('u')] = 0B1011,
  [WC_C('v')] = 0B10111,
  [WC_C('w')] = 0B1001,
  [WC_C('x')] = 0B10110,
  [WC_C('y')] = 0B10010,
  [WC_C('z')] = 0B11100,

  [WC_C('ä')] = 0B10101,
  [WC_C('á')] = 0B101001,
  [WC_C('å')] = 0B101001,
  [WC_C('é')] = 0B111011,
  [WC_C('ñ')] = 0B100100,
  [WC_C('ö')] = 0B11000,
  [WC_C('ü')] = 0B10011,

  [WC_C('0')] = 0B100000,
  [WC_C('1')] = 0B100001,
  [WC_C('2')] = 0B100011,
  [WC_C('3')] = 0B100111,
  [WC_C('4')] = 0B101111,
  [WC_C('5')] = 0B111111,
  [WC_C('6')] = 0B111110,
  [WC_C('7')] = 0B111100,
  [WC_C('8')] = 0B111000,
  [WC_C('9')] = 0B110000,

  [WC_C('.')] = 0B1010101,
  [WC_C(',')] = 0B1001100,
  [WC_C('?')] = 0B1110011,
  [WC_C('!')] = 0B1001010,
  [WC_C(':')] = 0B1111000,
  [WC_C('\'')] = 0B1100001,
  [WC_C('"')] = 0B1101101,
  [WC_C('(')] = 0B110010,
  [WC_C(')')] = 0B1010010,
  [WC_C('=')] = 0B101110,
  [WC_C('+')] = 0B110101,
  [WC_C('-')] = 0B1011110,
  [WC_C('/')] = 0B110110,
  [WC_C('&')] = 0B111101,
  [WC_C('@')] = 0B1101001,

  [0] = 0
};

MorsePattern
getMorsePattern (wchar_t character) {
  character = towlower(character);
  if (character < 0) return 0;
  if (character >= ARRAY_COUNT(morsePatterns)) return 0;
  return morsePatterns[character];
}

struct  MorseObjectStruct {
  struct {
    unsigned int frequency;
    unsigned int unit;
  } parameters;

  struct {
    unsigned wasSpace:1;
  } state;

  struct {
    ToneElement *array;
    size_t size;
    size_t count;
  } elements;
};

static int
addMorseElement (const ToneElement *element, MorseObject *morse) {
  if (morse->elements.count == morse->elements.size) {
    size_t newSize = morse->elements.size? (morse->elements.size << 1): 0X10;
    ToneElement *newArray = realloc(morse->elements.array, (newSize * sizeof(*newArray)));

    if (!newArray) {
      logMallocError();
      return 0;
    }

    morse->elements.array = newArray;
    morse->elements.size = newSize;
  }

  morse->elements.array[morse->elements.count++] = *element;
  return 1;
}

static int
addMorseMark (unsigned int units, MorseObject *morse) {
  ToneElement tone = TONE_PLAY((morse->parameters.unit * units), morse->parameters.frequency);
  return addMorseElement(&tone, morse);;
}

static int
addMorseGap (unsigned int units, MorseObject *morse) {
  ToneElement tone = TONE_REST((morse->parameters.unit * units));
  return addMorseElement(&tone, morse);;
}

int
addMorsePattern (MorsePattern pattern, MorseObject *morse) {
  if (pattern) {
    int addGap = 0;

    while (pattern != 0B1) {
      if (!addGap) {
        addGap = 1;
      } else if (!addMorseGap(MORSE_UNITS_GAP_SYMBOL, morse)) {
        return 0;
      }

      unsigned int units = (pattern & 0B1)? MORSE_UNITS_MARK_SHORT: MORSE_UNITS_MARK_LONG;
      if (!addMorseMark(units, morse)) return 0;
      pattern >>= 1;
    }
  }

  return 1;
}

int
addMorseCharacter (wchar_t character, MorseObject *morse) {
  if (!iswspace(character)) {
    if (morse->state.wasSpace) {
      morse->state.wasSpace = 0;
    } else if (!addMorseGap(MORSE_UNITS_GAP_LETTER, morse)) {
      return 0;
    }

    if (!addMorsePattern(getMorsePattern(character), morse)) return 0;
  } else if (!morse->state.wasSpace) {
    morse->state.wasSpace = 1;
    if (!addMorseGap(MORSE_UNITS_GAP_WORD, morse)) return 0;
  }

  return 1;
}

int
addMorseCharacters (const wchar_t *characters, size_t count, MorseObject *morse) {
  const wchar_t *character = characters;
  const wchar_t *end = character + count;

  while (character < end) {
    if (!addMorseCharacter(*character++, morse)) return 0;
  }

  return 1;
}

int
addMorseString (const char *string, MorseObject *morse) {
  size_t size = strlen(string) + 1;
  wchar_t characters[size];

  const char *byte = string;
  wchar_t *end = characters;

  convertUtf8ToWchars(&byte, &end, size);
  return addMorseCharacters(characters, (end - characters), morse);
}

int
playMorsePatterns (MorseObject *morse) {
  {
    ToneElement element = TONE_STOP();
    if (!addMorseElement(&element, morse)) return 0;
  }

  tunePlayTones(morse->elements.array);
  tuneSynchronize();
  return 1;
}

void *
newMorseObject (void) {
  MorseObject *morse;

  if ((morse = malloc(sizeof(*morse)))) {
    memset(morse, 0, sizeof(*morse));

    morse->parameters.frequency = 440;
    morse->parameters.unit = 50;

    morse->state.wasSpace = 1;

    morse->elements.array = NULL;
    morse->elements.size = 0;
    morse->elements.count = 0;

    return morse;
  } else {
    logMallocError();
  }

  return NULL;
}

void
destroyMorseObject (MorseObject *morse) {
  if (morse->elements.array) free(morse->elements.array);
  free(morse);
}
