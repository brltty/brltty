/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/*
 * unicode.c
 * $Id$
 * by Hans Schou
 *
 * Unicode names:
 * These names identifies all characters which is used
 * in the BRLTTY community. In any discussion this list
 * can be used to describe any character.
 *
 * Unicode defines all characters in 16 bit and there is
 * then no need to know the codepage. Codepage 437 and
 * ISO-8859-1 is only 8 bit tables and there can be no
 * full conversion between those two tables.
 *
 * Example:
 * The character 'a' has the unicode numer 0x0061.
 * Most western countries agrees on mapping the letter
 * 'a' to braille dots-1. Dots-1 in the unicode standard is
 * named: 0x2801 braille pattern dots-1.
 * In that case we have the mapping
 * unum:0x0061 bnum:0x2801
 *
 * With this mapping all blinds in the western countries
 * can exchange files and emails with seeing people, if
 * the seeing people uses some of those standard codepages
 * like CP437 or ISO-8859-1.
 *
 * Now, if a seeing person using codepage ISO-8859-1 and
 * typing the letter ø 0x00f8 named latin small letter o slash,
 * the receiver which is using codepage CP437 will not see
 * character but will instead see 0x00b0 a degree sign.
 * Because they do not use the same codepage they can not
 * communicate clearly.
 *
 * The same problem occours to blind people when they are
 * using different definitions of dots. Dots-1 is widely
 * accepted to be an 'a' but dots-6 is not very well defined.
 * In a case where two blind people both uses ISO-8859-1
 * and they are mapping the full table to there local
 * braille, there will be no problem. If one person uses
 * dots-5 for ø and the other uses dots-6 for ø, they will
 * then fully understand each other because of the local
 * mapping.
 *
 * ISO-8859-1 is quite good for most things. It is widely
 * used by seeing people and it works well under Linux.
 * But as times goes on new characters comes. One new 
 * character we got in year 1999 is the euro sign, a symbol
 * to represent the euro currency which is new. To use this
 * character seeing people needs to move over to ISO-8859-15
 * and Microsoft Windows users will have to use another
 * new codepage. To make it all complicated Microsoft uses
 * the code 0x80 for the euro sign where ISO-8859-15 uses
 * 0xA4 for the euro sign. Microsoft calls their standard
 * ISO-8859-1 compatible which it can not be if they have
 * the euro sign, as the euro sign is not defined in ISO-8859-1.
 * 
 * To learn more about ISO-8859-1 read:
 * http://czyborra.com/charsets/iso8859.html
 *
 * To solve all this mess we have implemented a look-up function
 * in BRLTTY. First you have to select a codepage to use.
 * The codepage could be ISO-8859-1 where all characters you
 * read will be treated as this codepage. If you see some strange
 * characters you don't know what is, then move the cursor to that
 * character and press XX chord and the braille display will tell
 * you the Unicode name of that character. Press any key to clear
 * the braille display and return to text.
 *
 * Example:
 * You read "Stéphane" on the braille display. Move the cursor to
 * é and press XX chord. The braille display will now show:
 * "é latin small letter e acute, iso-8859-1:0xe9 unicode:0x00e9
 * Could be substituted with e. The é is often used in french 
 *
 * If user does not have é in the braille_table an e will be
 * displayed instead. However, if the XX chord command is pressed
 * on this letter the answer will still be "latin small letter e acute".
 *
 * Conversion:
 * Each codepage can show up to 256 characters. CP437, ISO-8859-xx
 * can show different characters. This means that there will always
 * be some characters which can not be seen by either blind or seeing
 * people. On the other hand is there a different braille standard
 * for each country. To make a generic conversion from any codepage
 * to any braille standard, a character from a file is first converted
 * to unicode, then to the local braille standard, and then to the
 * standard used by the specific braille device.
 *
 * text -> codepage -> unicode -> local-braille -> device
 *
 * In this way we can handle:
 * - same presentation on all devices
 * - change of codepages, latin, russian, greek
 *
 * Examples:
 * A character is read from a file. The hexadecimal value is 0x61.
 * First it is converted according to the codepage over to Unicode.
 * If using ISO-8859-1 this character is an 'a' and will be mapped to
 * 0x0061 in Unicode. With the local braille conversion table it
 * is converted to dot-code where 0x0061 maps to 0x2801 in the
 * braille table which also known as dots-1. If the device connected
 * is a BrailleLite, the character will be converted to 0x61 which
 * actually what the character was from start. However, this is not
 * always the case.
 *
 * The character 0xf8 is read from file. The codepage is ISO-8859-1
 * which gives that the character is an 'ø' (small o slash). Mapped
 * to Unicode it is 0x00f8. The local braille table is danish and
 * 0x00f8 will be mapped to braille 0x282a dots-246. With a 
 * BrailleLite 0x282a dots-246 should be converted to 0x7b to
 * represent dots-246.
 *
 * A file is received which is using codepage 850. The sender
 * intended to write an 'ø' (o slash), which in codepage 850 has the
 * value 0x9b. If 0x9b is converted with the ISO-8859-1 table
 * it will converted to a wrong value. The braille user now has
 * to change codepage to CP850 and then the Unicode will be converted
 * to 0x00f8, an o slash which was what the sender intended. As the
 * example above, 0x00f8 will be converted to dots-246 if the user is
 * using a danish braille standard.
 *
 * If a file with an 'ø' is received by an american, he will get some
 * conversion but will not know what character it is. The character
 * could e.g. be converted to dots-678 which he do not know what is.
 * He could then move the cursor to the character and press the
 * XX chord and get a description of the character.
 *
 * The tables and what they convert:
 * iso_8859_1->table[0xf8] == 0x00f8 in Unicode.
 * braille_da[n]->unum{0x00f8} == 0x282a in braille (dots-246).
 * BrailleLite[0x282a & 0xff] == 0x12.
 *
 * A one-line conversion of the above:
 * 0x12 == BrailleLite[0xff & braille_da[iso_8859_1->table[0xf8]]]
 * All the tables is loaded files, so internally we just do:
 * 0x12 == text_to_braille[0xf8];
 * after the user has selected codepage, local-braille and device.
 *
 * The Unicode description can be found by:
 * unicodeTable[iso_8859_1[0xf8]]->name == "latin small letter o slash";
 * (well, I binary search on unicodeTable[n]->unum migth be better)
 *
 * There is several tables here.
 * 1. unicodeTable
 *    A list which defines all characters.
 *    unum: unique identifier in hexadecimal.
 *    name: the name of the character in letters
 *    desc: an optional description of the character
 * 2. unicode_iso8859_1
 *    A mapping between unicode and ISO-8859-1.
 * 3. table_braille_<language>
 *    unum:0x0041, name:"latin capital letter a"
 */

#include <stdlib.h>
#include "unicode.h"

UnicodeEntry unicodeTable[] = {
  #include "unicode-table.h"
};
static const unsigned int unicodeCount = sizeof(unicodeTable) / sizeof(unicodeTable[0]);

static const CodePage iso_8859_1 = {
  name:"iso-8859-1",
  desc:"Latin (Western)",
  table:{
    #include "iso-8859-1.h"
  }
};

static const CodePage iso_8859_2 = {
  name:"iso-8859-2",
  desc:"Latin (Central European)",
  table:{
    #include "iso-8859-2.h"
  }
};

static const CodePage iso_8859_3 = {
  name:"iso-8859-3",
  desc:"Latin (South European)",
  table:{
    #include "iso-8859-3.h"
  }
};

static const CodePage iso_8859_4 = {
  name:"iso-8859-4",
  desc:"Latin (North European)",
  table:{
    #include "iso-8859-4.h"
  }
};

static const CodePage iso_8859_5 = {
  name:"iso-8859-5",
  desc:"Latin/Cyrillic (Slavic)",
  table:{
    #include "iso-8859-5.h"
  }
};

static const CodePage iso_8859_6 = {
  name:"iso-8859-6",
  desc:"Latin/Arabic (Arabic)",
  table:{
    #include "iso-8859-6.h"
  }
};

static const CodePage iso_8859_7 = {
  name:"iso-8859-7",
  desc:"Latin/Greek (modern Greek)",
  table:{
    #include "iso-8859-7.h"
  }
};

static const CodePage iso_8859_8 = {
  name:"iso-8859-8",
  desc:"Latin/Hebrew (Hebrew and Yiddish)",
  table:{
    #include "iso-8859-8.h"
  }
};

static const CodePage iso_8859_9 = {
  name:"iso-8859-9",
  desc:"Latin (Turkish)",
  table:{
    #include "iso-8859-9.h"
  }
};

static const CodePage iso_8859_10 = {
  name:"iso-8859-10",
  desc:"Latin (Nordic)",
  table:{
    #include "iso-8859-10.h"
  }
};

static const CodePage iso_8859_13 = {
  name:"iso-8859-13",
  desc:"Latin (Baltic Rim)",
  table:{
    #include "iso-8859-13.h"
  }
};

static const CodePage iso_8859_14 = {
  name:"iso-8859-14",
  desc:"Latin (Celtic)",
  table:{
    #include "iso-8859-14.h"
  }
};

static const CodePage iso_8859_15 = {
  name:"iso-8859-15",
  desc:"Latin (Euro)",
  table:{
    #include "iso-8859-15.h"
  }
};

static const CodePage iso_8859_16 = {
  name:"iso-8859-16",
  desc:"Latin (collection of languages)",
  table:{
    #include "iso-8859-16.h"
  }
};

static const CodePage cp_37 = {
  name:"cp-37",
  desc:"EBCDIC: US/Canada English",
  table:{
    #include "cp-37.h"
  }
};

static const CodePage cp_424 = {
  name:"cp-424",
  desc:"EBCDIC: Hebrew",
  table:{
    #include "cp-424.h"
  }
};

static const CodePage cp_437 = {
  name:"cp-437",
  desc:"US English",
  table:{
    #include "cp-437.h"
  }
};

static const CodePage cp_500 = {
  name:"cp-500",
  desc:"EBCDIC: Belgium/Switzerland",
  table:{
    #include "cp-500.h"
  }
};

static const CodePage cp_737 = {
  name:"cp-737",
  desc:"Greek (2)",
  table:{
    #include "cp-737.h"
  }
};

static const CodePage cp_775 = {
  name:"cp-775",
  desc:"Baltic Rim",
  table:{
    #include "cp-775.h"
  }
};

static const CodePage cp_850 = {
  name:"cp-850",
  desc:"Multilingual (Latin 1)",
  table:{
    #include "cp-850.h"
  }
};

static const CodePage cp_852 = {
  name:"cp-852",
  desc:"Slavic/Eastern Europe (Latin 2)",
  table:{
    #include "cp-852.h"
  }
};

static const CodePage cp_855 = {
  name:"cp-855",
  desc:"Cyrillic (1)",
  table:{
    #include "cp-855.h"
  }
};

static const CodePage cp_856 = {
  name:"cp-856",
  desc:"EBCDIC:",
  table:{
    #include "cp-856.h"
  }
};

static const CodePage cp_857 = {
  name:"cp-857",
  desc:"Turkish",
  table:{
    #include "cp-857.h"
  }
};

static const CodePage cp_860 = {
  name:"cp-860",
  desc:"Portugese",
  table:{
    #include "cp-860.h"
  }
};

static const CodePage cp_861 = {
  name:"cp-861",
  desc:"Icelandic",
  table:{
    #include "cp-861.h"
  }
};

static const CodePage cp_862 = {
  name:"cp-862",
  desc:"Hebrew",
  table:{
    #include "cp-862.h"
  }
};

static const CodePage cp_863 = {
  name:"cp-863",
  desc:"French Canadian",
  table:{
    #include "cp-863.h"
  }
};

static const CodePage cp_864 = {
  name:"cp-864",
  desc:"Arabic/Middle East",
  table:{
    #include "cp-864.h"
  }
};

static const CodePage cp_865 = {
  name:"cp-865",
  desc:"Nordic (Norwegian, Danish)",
  table:{
    #include "cp-865.h"
  }
};

static const CodePage cp_866 = {
  name:"cp-866",
  desc:"Russian (Cyrillic 2)",
  table:{
    #include "cp-866.h"
  }
};

static const CodePage cp_869 = {
  name:"cp-869",
  desc:"Greek (1)",
  table:{
    #include "cp-869.h"
  }
};

static const CodePage cp_874 = {
  name:"cp-874",
  desc:"Thailand",
  table:{
    #include "cp-874.h"
  }
};

static const CodePage cp_875 = {
  name:"cp-875",
  desc:"EBCDIC: Greek",
  table:{
    #include "cp-875.h"
  }
};

static const CodePage cp_1006 = {
  name:"cp-1006",
  desc:"EBCDIC: Arabic",
  table:{
    #include "cp-1006.h"
  }
};

static const CodePage cp_1026 = {
  name:"cp-1026",
  desc:"EBCDIC: Turkish (Latin 5)",
  table:{
    #include "cp-1026.h"
  }
};

static const CodePage cp_1250 = {
  name:"cp-1250",
  desc:"MSWIN: Eastern Europe (Latin 2)",
  table:{
    #include "cp-1250.h"
  }
};

static const CodePage cp_1251 = {
  name:"cp-1251",
  desc:"MSWIN: Cyrillic",
  table:{
    #include "cp-1251.h"
  }
};

static const CodePage cp_1252 = {
  name:"cp-1252",
  desc:"MSWIN: English/Western Europe (Latin 1)",
  table:{
    #include "cp-1252.h"
  }
};

static const CodePage cp_1253 = {
  name:"cp-1253",
  desc:"MSWIN: Greek (GRC)",
  table:{
    #include "cp-1253.h"
  }
};

static const CodePage cp_1254 = {
  name:"cp-1254",
  desc:"MSWIN: Turkish",
  table:{
    #include "cp-1254.h"
  }
};

static const CodePage cp_1255 = {
  name:"cp-1255",
  desc:"MSWIN: Hebrew",
  table:{
    #include "cp-1255.h"
  }
};

static const CodePage cp_1256 = {
  name:"cp-1256",
  desc:"MSWIN: Arabic",
  table:{
    #include "cp-1256.h"
  }
};

static const CodePage cp_1257 = {
  name:"cp-1257",
  desc:"MSWIN: Baltic (Estonian, Latvian, Lithuanian)",
  table:{
    #include "cp-1257.h"
  }
};

static const CodePage cp_1258 = {
  name:"cp-1258",
  desc:"MSWIN: Vietnamese",
  table:{
    #include "cp-1258.h"
  }
};

static const CodePage koi8_r = {
  name:"koi8-r",
  desc:"Russian",
  table:{
    #include "koi8-r.h"
  }
};

static const CodePage koi8_ru = {
  name:"koi8-ru",
  desc:"Ukrainian, Russian, Belorus",
  table:{
    #include "koi8-ru.h"
  }
};

static const CodePage koi8_u = {
  name:"koi8-u",
  desc:"Ukrainian",
  table:{
    #include "koi8-u.h"
  }
};

const CodePage *const codePageTable[] = {
  &iso_8859_1,
  &iso_8859_2,
  &iso_8859_3,
  &iso_8859_4,
  &iso_8859_5,
  &iso_8859_6,
  &iso_8859_7,
  &iso_8859_8,
  &iso_8859_9,
  &iso_8859_10,
  &iso_8859_13,
  &iso_8859_14,
  &iso_8859_15,
  &iso_8859_16,
  &cp_37,
  &cp_424,
  &cp_437,
  &cp_500,
  &cp_737,
  &cp_775,
  &cp_850,
  &cp_852,
  &cp_855,
  &cp_856,
  &cp_857,
  &cp_860,
  &cp_861,
  &cp_862,
  &cp_863,
  &cp_864,
  &cp_865,
  &cp_866,
  &cp_869,
  &cp_874,
  &cp_875,
  &cp_1006,
  &cp_1026,
  &cp_1250,
  &cp_1251,
  &cp_1252,
  &cp_1253,
  &cp_1254,
  &cp_1255,
  &cp_1256,
  &cp_1257,
  &cp_1258,
  &koi8_r,
  &koi8_ru,
  &koi8_u
};
const unsigned int codePageCount = sizeof(codePageTable) / sizeof(codePageTable[0]);

const UnicodeEntry *getUnicodeEntry (unsigned int unum) {
  unsigned int first = 0;
  unsigned int last = unicodeCount - 1;
  /* do a binary search for u in unicodeNames[i]->unum */
  while (first <= last) {
    int current = (first + last) / 2;
    const UnicodeEntry *unicode = &unicodeTable[current];
    if (unum == unicode->unum) return unicode;
    if (unum > unicode->unum)
      first = current + 1;
    else if (current == 0)
      break;
    else
      last = current - 1;
  }
  return NULL;
}

const CodePage *getCodePage (const char *name) {
  int index;
  for (index=0; index<codePageCount; ++index) {
    const CodePage *page = codePageTable[index];
    if (strcmp(name, page->name) == 0) return page;
  }
  return NULL;
}
