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

/* Derived from linux-2.2.17/fs/nls/nls_cp737.c by Dave Mielke <dave@mielke.cc>. */

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
[0x25]0x0025, // percent sign
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
[0x80]0x0391, // greek capital letter alpha
[0x81]0x0392, // greek capital letter beta
[0x82]0x0393, // greek capital letter gamma
[0x83]0x0394, // greek capital letter delta
[0x84]0x0395, // greek capital letter epsilon
[0x85]0x0396, // greek capital letter zeta
[0x86]0x0397, // greek capital letter eta
[0x87]0x0398, // greek capital letter theta
[0x88]0x0399, // greek capital letter iota
[0x89]0x039a, // greek capital letter kappa
[0x8a]0x039b, // greek capital letter lamda
[0x8b]0x039c, // greek capital letter mu
[0x8c]0x039d, // greek capital letter nu
[0x8d]0x039e, // greek capital letter xi
[0x8e]0x039f, // greek capital letter omicron
[0x8f]0x03a0, // greek capital letter pi
[0x90]0x03a1, // greek capital letter rho
[0x91]0x03a3, // greek capital letter sigma
[0x92]0x03a4, // greek capital letter tau
[0x93]0x03a5, // greek capital letter upsilon
[0x94]0x03a6, // greek capital letter phi
[0x95]0x03a7, // greek capital letter chi
[0x96]0x03a8, // greek capital letter psi
[0x97]0x03a9, // greek capital letter omega
[0x98]0x03b1, // greek small letter alpha
[0x99]0x03b2, // greek small letter beta
[0x9a]0x03b3, // greek small letter gamma
[0x9b]0x03b4, // greek small letter delta
[0x9c]0x03b5, // greek small letter epsilon
[0x9d]0x03b6, // greek small letter zeta
[0x9e]0x03b7, // greek small letter eta
[0x9f]0x03b8, // greek small letter theta
[0xa0]0x03b9, // greek small letter iota
[0xa1]0x03ba, // greek small letter kappa
[0xa2]0x03bb, // greek small letter lamda
[0xa3]0x03bc, // greek small letter mu
[0xa4]0x03bd, // greek small letter nu
[0xa5]0x03be, // greek small letter xi
[0xa6]0x03bf, // greek small letter omicron
[0xa7]0x03c0, // greek small letter pi
[0xa8]0x03c1, // greek small letter rho
[0xa9]0x03c3, // greek small letter sigma
[0xaa]0x03c2, // greek small letter final sigma
[0xab]0x03c4, // greek small letter tau
[0xac]0x03c5, // greek small letter upsilon
[0xad]0x03c6, // greek small letter phi
[0xae]0x03c7, // greek small letter chi
[0xaf]0x03c8, // greek small letter psi
[0xb0]0x2591, // light shade
[0xb1]0x2592, // medium shade
[0xb2]0x2593, // dark shade
[0xb3]0x2502, // box drawings light vertical
[0xb4]0x2524, // box drawings light vertical and left
[0xb5]0x2561, // box drawings vertical single and left double
[0xb6]0x2562, // box drawings vertical double and left single
[0xb7]0x2556, // box drawings down double and left single
[0xb8]0x2555, // box drawings down single and left double
[0xb9]0x2563, // box drawings double vertical and left
[0xba]0x2551, // box drawings double vertical
[0xbb]0x2557, // box drawings double down and left
[0xbc]0x255d, // box drawings double up and left
[0xbd]0x255c, // box drawings up double and left single
[0xbe]0x255b, // box drawings up single and left double
[0xbf]0x2510, // box drawings light down and left
[0xc0]0x2514, // box drawings light up and right
[0xc1]0x2534, // box drawings light up and horizontal
[0xc2]0x252c, // box drawings light down and horizontal
[0xc3]0x251c, // box drawings light vertical and right
[0xc4]0x2500, // box drawings light horizontal
[0xc5]0x253c, // box drawings light vertical and horizontal
[0xc6]0x255e, // box drawings vertical single and right double
[0xc7]0x255f, // box drawings vertical double and right single
[0xc8]0x255a, // box drawings double up and right
[0xc9]0x2554, // box drawings double down and right
[0xca]0x2569, // box drawings double up and horizontal
[0xcb]0x2566, // box drawings double down and horizontal
[0xcc]0x2560, // box drawings double vertical and right
[0xcd]0x2550, // box drawings double horizontal
[0xce]0x256c, // box drawings double vertical and horizontal
[0xcf]0x2567, // box drawings up single and horizontal double
[0xd0]0x2568, // box drawings up double and horizontal single
[0xd1]0x2564, // box drawings down single and horizontal double
[0xd2]0x2565, // box drawings down double and horizontal single
[0xd3]0x2559, // box drawings up double and right single
[0xd4]0x2558, // box drawings up single and right double
[0xd5]0x2552, // box drawings down single and right double
[0xd6]0x2553, // box drawings down double and right single
[0xd7]0x256b, // box drawings vertical double and horizontal single
[0xd8]0x256a, // box drawings vertical single and horizontal double
[0xd9]0x2518, // box drawings light up and left
[0xda]0x250c, // box drawings light down and right
[0xdb]0x2588, // full block
[0xdc]0x2584, // lower half block
[0xdd]0x258c, // left half block
[0xde]0x2590, // right half block
[0xdf]0x2580, // upper half block
[0xe0]0x03c9, // greek small letter omega
[0xe1]0x03ac, // greek small letter alpha with tonos
[0xe2]0x03ad, // greek small letter epsilon with tonos
[0xe3]0x03ae, // greek small letter eta with tonos
[0xe4]0x03ca, // greek small letter iota with dialytika
[0xe5]0x03af, // greek small letter iota with tonos
[0xe6]0x03cc, // greek small letter omicron with tonos
[0xe7]0x03cd, // greek small letter upsilon with tonos
[0xe8]0x03cb, // greek small letter upsilon with dialytika
[0xe9]0x03ce, // greek small letter omega with tonos
[0xea]0x0386, // greek capital letter alpha with tonos
[0xeb]0x0388, // greek capital letter epsilon with tonos
[0xec]0x0389, // greek capital letter eta with tonos
[0xed]0x038a, // greek capital letter iota with tonos
[0xee]0x038c, // greek capital letter omicron with tonos
[0xef]0x038e, // greek capital letter upsilon with tonos
[0xf0]0x038f, // greek capital letter omega with tonos
[0xf1]0x00b1, // plus-minus sign
[0xf2]0x2265, // greater-than or equal to
[0xf3]0x2264, // less-than or equal to
[0xf4]0x03aa, // greek capital letter iota with dialytika
[0xf5]0x03ab, // greek capital letter upsilon with dialytika
[0xf6]0x00f7, // division sign
[0xf7]0x2248, // almost equal to
[0xf8]0x00b0, // degree sign
[0xf9]0x2219, // bullet operator
[0xfa]0x00b7, // middle dot
[0xfb]0x221a, // square root
[0xfc]0x207f, // superscript latin small letter n
[0xfd]0x00b2, // superscript two
[0xfe]0x25a0, // black square
[0xff]0x00a0  // no-break space
