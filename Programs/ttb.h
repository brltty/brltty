/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_TTB
#define BRLTTY_INCLUDED_TTB

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct TextTableStruct TextTable;

extern TextTable *textTable;

extern TextTable *compileTextTable (const char *name);
extern void destroyTextTable (TextTable *table);

extern char *ensureTextTableExtension (const char *path);

extern unsigned char convertCharacterToDots (TextTable *table, wchar_t character);
extern wchar_t convertDotsToCharacter (TextTable *table, unsigned char dots);

#define UNICODE_REPLACEMENT_CHARACTER 0XFFFD
#define UNICODE_BRAILLE_ROW 0X2800

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_TTB */
