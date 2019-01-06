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

#ifndef BRLTTY_INCLUDED_MORSE
#define BRLTTY_INCLUDED_MORSE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef uint8_t MorsePattern;
extern MorsePattern getMorsePattern (wchar_t character);

#define MORSE_UNITS_MARK_SHORT 1
#define MORSE_UNITS_MARK_LONG  3
#define MORSE_UNITS_GAP_SYMBOL 1
#define MORSE_UNITS_GAP_LETTER 3
#define MORSE_UNITS_GAP_WORD   7

typedef struct MorseObjectStruct MorseObject;
extern void *newMorseObject (void);
extern void destroyMorseObject (MorseObject *morse);

extern int addMorseString (const char *string, MorseObject *morse);
extern int addMorseCharacters (const wchar_t *characters, size_t count, MorseObject *morse);
extern int addMorseCharacter (wchar_t character, MorseObject *morse);
extern int addMorsePattern (MorsePattern pattern, MorseObject *morse);
extern int playMorsePatterns (MorseObject *morse);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_MORSE */
