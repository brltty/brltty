/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* Derived from linux-2.2.17/fs/nls/nls_cp864.c by Dave Mielke <dave@mielke.cc>. */

[0x00]0x0000, // null
[0x01]0x0001, // start of heading
[0x02]0x0002, // start of text
[0x03]0x0003, // end of text
[0x04]0x0004, // end of transmission
[0x05]0x0005, // enquiry
[0x06]0x0006, // acknowledge
[0x07]0x0007, // bell
[0x08]0x0008, // backspace
[0x09]0x0009, // horizontal tabulation
[0x0a]0x000a, // line feed
[0x0b]0x000b, // vertical tabulation
[0x0c]0x000c, // form feed
[0x0d]0x000d, // carriage return
[0x0e]0x000e, // shift out
[0x0f]0x000f, // shift in
[0x10]0x0010, // data link escape
[0x11]0x0011, // device control one
[0x12]0x0012, // device control two
[0x13]0x0013, // device control three
[0x14]0x0014, // device control four
[0x15]0x0015, // negative acknowledge
[0x16]0x0016, // synchronous idle
[0x17]0x0017, // end of transmission block
[0x18]0x0018, // cancel
[0x19]0x0019, // end of medium
[0x1a]0x001a, // substitute
[0x1b]0x001b, // escape
[0x1c]0x001c, // file separator
[0x1d]0x001d, // group separator
[0x1e]0x001e, // record separator
[0x1f]0x001f, // unit separator
[0x20]0x0020, // space
[0x21]0x0021, // exclamation mark
[0x22]0x0022, // quotation mark
[0x23]0x0023, // number sign
[0x24]0x0024, // dollar sign
[0x25]0x066a, // arabic percent sign
[0x26]0x0026, // ampersand
[0x27]0x0027, // apostrophe
[0x28]0x0028, // left parenthesis
[0x29]0x0029, // right parenthesis
[0x2a]0x002a, // asterisk
[0x2b]0x002b, // plus sign
[0x2c]0x002c, // comma
[0x2d]0x002d, // hyphen-minus
[0x2e]0x002e, // full stop
[0x2f]0x002f, // solidus
[0x30]0x0030, // digit zero
[0x31]0x0031, // digit one
[0x32]0x0032, // digit two
[0x33]0x0033, // digit three
[0x34]0x0034, // digit four
[0x35]0x0035, // digit five
[0x36]0x0036, // digit six
[0x37]0x0037, // digit seven
[0x38]0x0038, // digit eight
[0x39]0x0039, // digit nine
[0x3a]0x003a, // colon
[0x3b]0x003b, // semicolon
[0x3c]0x003c, // less-than sign
[0x3d]0x003d, // equals sign
[0x3e]0x003e, // greater-than sign
[0x3f]0x003f, // question mark
[0x40]0x0040, // commercial at
[0x41]0x0041, // latin capital letter a
[0x42]0x0042, // latin capital letter b
[0x43]0x0043, // latin capital letter c
[0x44]0x0044, // latin capital letter d
[0x45]0x0045, // latin capital letter e
[0x46]0x0046, // latin capital letter f
[0x47]0x0047, // latin capital letter g
[0x48]0x0048, // latin capital letter h
[0x49]0x0049, // latin capital letter i
[0x4a]0x004a, // latin capital letter j
[0x4b]0x004b, // latin capital letter k
[0x4c]0x004c, // latin capital letter l
[0x4d]0x004d, // latin capital letter m
[0x4e]0x004e, // latin capital letter n
[0x4f]0x004f, // latin capital letter o
[0x50]0x0050, // latin capital letter p
[0x51]0x0051, // latin capital letter q
[0x52]0x0052, // latin capital letter r
[0x53]0x0053, // latin capital letter s
[0x54]0x0054, // latin capital letter t
[0x55]0x0055, // latin capital letter u
[0x56]0x0056, // latin capital letter v
[0x57]0x0057, // latin capital letter w
[0x58]0x0058, // latin capital letter x
[0x59]0x0059, // latin capital letter y
[0x5a]0x005a, // latin capital letter z
[0x5b]0x005b, // left square bracket
[0x5c]0x005c, // reverse solidus
[0x5d]0x005d, // right square bracket
[0x5e]0x005e, // circumflex accent
[0x5f]0x005f, // low line
[0x60]0x0060, // grave accent
[0x61]0x0061, // latin small letter a
[0x62]0x0062, // latin small letter b
[0x63]0x0063, // latin small letter c
[0x64]0x0064, // latin small letter d
[0x65]0x0065, // latin small letter e
[0x66]0x0066, // latin small letter f
[0x67]0x0067, // latin small letter g
[0x68]0x0068, // latin small letter h
[0x69]0x0069, // latin small letter i
[0x6a]0x006a, // latin small letter j
[0x6b]0x006b, // latin small letter k
[0x6c]0x006c, // latin small letter l
[0x6d]0x006d, // latin small letter m
[0x6e]0x006e, // latin small letter n
[0x6f]0x006f, // latin small letter o
[0x70]0x0070, // latin small letter p
[0x71]0x0071, // latin small letter q
[0x72]0x0072, // latin small letter r
[0x73]0x0073, // latin small letter s
[0x74]0x0074, // latin small letter t
[0x75]0x0075, // latin small letter u
[0x76]0x0076, // latin small letter v
[0x77]0x0077, // latin small letter w
[0x78]0x0078, // latin small letter x
[0x79]0x0079, // latin small letter y
[0x7a]0x007a, // latin small letter z
[0x7b]0x007b, // left curly bracket
[0x7c]0x007c, // vertical line
[0x7d]0x007d, // right curly bracket
[0x7e]0x007e, // tilde
[0x7f]0x007f, // delete
[0x80]0x00b0, // degree sign
[0x81]0x00b7, // middle dot
[0x82]0x2219, // bullet operator
[0x83]0x221a, // square root
[0x84]0x2592, // medium shade
[0x85]0x2500, // box drawings light horizontal
[0x86]0x2502, // box drawings light vertical
[0x87]0x253c, // box drawings light vertical and horizontal
[0x88]0x2524, // box drawings light vertical and left
[0x89]0x252c, // box drawings light down and horizontal
[0x8a]0x251c, // box drawings light vertical and right
[0x8b]0x2534, // box drawings light up and horizontal
[0x8c]0x2510, // box drawings light down and left
[0x8d]0x250c, // box drawings light down and right
[0x8e]0x2514, // box drawings light up and right
[0x8f]0x2518, // box drawings light up and left
[0x90]0x03b2, // greek small letter beta
[0x91]0x221e, // infinity
[0x92]0x03c6, // greek small letter phi
[0x93]0x00b1, // plus-minus sign
[0x94]0x00bd, // vulgar fraction one half
[0x95]0x00bc, // vulgar fraction one quarter
[0x96]0x2248, // almost equal to
[0x97]0x00ab, // left-pointing double angle quotation mark
[0x98]0x00bb, // right-pointing double angle quotation mark
[0x99]0xfef7, // arabic ligature lam with alef with hamza above isolated form
[0x9a]0xfef8, // arabic ligature lam with alef with hamza above final form
[0x9b]0x0000, // null
[0x9c]0x0000, // null
[0x9d]0xfefb, // arabic ligature lam with alef isolated form
[0x9e]0xfefc, // arabic ligature lam with alef final form
[0x9f]0x0000, // null
[0xa0]0x00a0, // no-break space
[0xa1]0x00ad, // soft hyphen
[0xa2]0xfe82, // arabic letter alef with madda above final form
[0xa3]0x00a3, // pound sign
[0xa4]0x00a4, // currency sign
[0xa5]0xfe84, // arabic letter alef with hamza above final form
[0xa6]0x0000, // null
[0xa7]0x0000, // null
[0xa8]0xfe8e, // arabic letter alef final form
[0xa9]0xfe8f, // arabic letter beh isolated form
[0xaa]0xfe95, // arabic letter teh isolated form
[0xab]0xfe99, // arabic letter theh isolated form
[0xac]0x060c, // arabic comma
[0xad]0xfe9d, // arabic letter jeem isolated form
[0xae]0xfea1, // arabic letter hah isolated form
[0xaf]0xfea5, // arabic letter khah isolated form
[0xb0]0x0660, // arabic-indic digit zero
[0xb1]0x0661, // arabic-indic digit one
[0xb2]0x0662, // arabic-indic digit two
[0xb3]0x0663, // arabic-indic digit three
[0xb4]0x0664, // arabic-indic digit four
[0xb5]0x0665, // arabic-indic digit five
[0xb6]0x0666, // arabic-indic digit six
[0xb7]0x0667, // arabic-indic digit seven
[0xb8]0x0668, // arabic-indic digit eight
[0xb9]0x0669, // arabic-indic digit nine
[0xba]0xfed1, // arabic letter feh isolated form
[0xbb]0x061b, // arabic semicolon
[0xbc]0xfeb1, // arabic letter seen isolated form
[0xbd]0xfeb5, // arabic letter sheen isolated form
[0xbe]0xfeb9, // arabic letter sad isolated form
[0xbf]0x061f, // arabic question mark
[0xc0]0x00a2, // cent sign
[0xc1]0xfe80, // arabic letter hamza isolated form
[0xc2]0xfe81, // arabic letter alef with madda above isolated form
[0xc3]0xfe83, // arabic letter alef with hamza above isolated form
[0xc4]0xfe85, // arabic letter waw with hamza above isolated form
[0xc5]0xfeca, // arabic letter ain final form
[0xc6]0xfe8b, // arabic letter yeh with hamza above initial form
[0xc7]0xfe8d, // arabic letter alef isolated form
[0xc8]0xfe91, // arabic letter beh initial form
[0xc9]0xfe93, // arabic letter teh marbuta isolated form
[0xca]0xfe97, // arabic letter teh initial form
[0xcb]0xfe9b, // arabic letter theh initial form
[0xcc]0xfe9f, // arabic letter jeem initial form
[0xcd]0xfea3, // arabic letter hah initial form
[0xce]0xfea7, // arabic letter khah initial form
[0xcf]0xfea9, // arabic letter dal isolated form
[0xd0]0xfeab, // arabic letter thal isolated form
[0xd1]0xfead, // arabic letter reh isolated form
[0xd2]0xfeaf, // arabic letter zain isolated form
[0xd3]0xfeb3, // arabic letter seen initial form
[0xd4]0xfeb7, // arabic letter sheen initial form
[0xd5]0xfebb, // arabic letter sad initial form
[0xd6]0xfebf, // arabic letter dad initial form
[0xd7]0xfec1, // arabic letter tah isolated form
[0xd8]0xfec5, // arabic letter zah isolated form
[0xd9]0xfecb, // arabic letter ain initial form
[0xda]0xfecf, // arabic letter ghain initial form
[0xdb]0x00a6, // broken bar
[0xdc]0x00ac, // not sign
[0xdd]0x00f7, // division sign
[0xde]0x00d7, // multiplication sign
[0xdf]0xfec9, // arabic letter ain isolated form
[0xe0]0x0640, // arabic tatweel
[0xe1]0xfed3, // arabic letter feh initial form
[0xe2]0xfed7, // arabic letter qaf initial form
[0xe3]0xfedb, // arabic letter kaf initial form
[0xe4]0xfedf, // arabic letter lam initial form
[0xe5]0xfee3, // arabic letter meem initial form
[0xe6]0xfee7, // arabic letter noon initial form
[0xe7]0xfeeb, // arabic letter heh initial form
[0xe8]0xfeed, // arabic letter waw isolated form
[0xe9]0xfeef, // arabic letter alef maksura isolated form
[0xea]0xfef3, // arabic letter yeh initial form
[0xeb]0xfebd, // arabic letter dad isolated form
[0xec]0xfecc, // arabic letter ain medial form
[0xed]0xfece, // arabic letter ghain final form
[0xee]0xfecd, // arabic letter ghain isolated form
[0xef]0xfee1, // arabic letter meem isolated form
[0xf0]0xfe7d, // arabic shadda medial form
[0xf1]0x0651, // arabic shadda
[0xf2]0xfee5, // arabic letter noon isolated form
[0xf3]0xfee9, // arabic letter heh isolated form
[0xf4]0xfeec, // arabic letter heh medial form
[0xf5]0xfef0, // arabic letter alef maksura final form
[0xf6]0xfef2, // arabic letter yeh final form
[0xf7]0xfed0, // arabic letter ghain medial form
[0xf8]0xfed5, // arabic letter qaf isolated form
[0xf9]0xfef5, // arabic ligature lam with alef with madda above isolated form
[0xfa]0xfef6, // arabic ligature lam with alef with madda above final form
[0xfb]0xfedd, // arabic letter lam isolated form
[0xfc]0xfed9, // arabic letter kaf isolated form
[0xfd]0xfef1, // arabic letter yeh isolated form
[0xfe]0x25a0, // black square
[0xff]0x0000  // null
