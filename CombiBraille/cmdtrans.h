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

/* Note: this table has three distinct sections:
 *   1. offsets 0x00-0x3f - the Braille dot keys:
 *           bit  5  4  3  2  1  0
 *           dot  3  2  1  4  5  6
 *   2. offsets 0x60-0x7f - the thumb keys:
 *           bit  4  3  2  1  0
 *           key  E  D  C  B  A
 *      (labelling A-E left to right)
 *   3. offsets 0x80 upwards - the cursor routing keys: 0x80 + keynumber
 *      (0 at the left, max. 85)
 */

/* 0X00 */ CMD_NOOP,      // no dots
/* 0X01 */ CMD_FWINRT,    // dots 6
/* 0X02 */ CMD_CSRVIS,    // dots 5
/* 0X03 */ CMD_LNEND,     // dots 5,6
/* 0X04 */ CMD_LNDN,      // dots 4
/* 0X05 */ CMD_CHRRT,     // dots 4,6
/* 0X06 */ CMD_WINDN,     // dots 4,5
/* 0X07 */ CMD_BOT,       // dots 4,5,6
/* 0X08 */ CMD_LNUP,      // dots 1
/* 0X09 */ CMD_MUTE,      // dots 1,6
/* 0X0A */ CMD_NOOP,      // dots 1,5
/* 0X0B */ CMD_SAY,       // dots 1,5,6
/* 0X0C */ CMD_CSRTRK,    // dots 1,4
/* 0X0D */ CMD_CAPBLINK,  // dots 1,4,6
/* 0X0E */ CMD_NOOP,      // dots 1,4,5
/* 0X0F */ CMD_NOOP,      // dots 1,4,5,6
/* 0X10 */ CMD_CSRBLINK,  // dots 2
/* 0X11 */ CMD_NOOP,      // dots 2,6
/* 0X12 */ CMD_CSRSIZE,   // dots 2,5
/* 0X13 */ CMD_NOOP,      // dots 2,5,6
/* 0X14 */ CMD_NOOP,      // dots 2,4
/* 0X15 */ CMD_HWINLT,    // dots 2,4,6
/* 0X16 */ CMD_SKPIDLNS,  // dots 2,4,5
/* 0X17 */ CMD_SLIDEWIN,  // dots 2,4,5,6
/* 0X18 */ CMD_WINUP,     // dots 1,2
/* 0X19 */ CMD_NOOP,      // dots 1,2,6
/* 0X1A */ CMD_HELP,      // dots 1,2,5
/* 0X1B */ CMD_NOOP,      // dots 1,2,5,6
/* 0X1C */ CMD_FREEZE,    // dots 1,2,4
/* 0X1D */ CMD_NOOP,      // dots 1,2,4,6
/* 0X1E */ CMD_CSRJMP,    // dots 1,2,4,5
/* 0X1F */ CMD_NOOP,      // dots 1,2,4,5,6
/* 0X20 */ CMD_FWINLT,    // dots 3
/* 0X21 */ CMD_HOME,      // dots 3,6
/* 0X22 */ CMD_NOOP,      // dots 3,5
/* 0X23 */ CMD_CUT_END,   // dots 3,5,6
/* 0X24 */ CMD_INFO,      // dots 3,4
/* 0X25 */ CMD_NOOP,      // dots 3,4,6
/* 0X26 */ CMD_NOOP,      // dots 3,4,5
/* 0X27 */ CMD_NOOP,      // dots 3,4,5,6
/* 0X28 */ CMD_CHRLT,     // dots 1,3
/* 0X29 */ CMD_NOOP,      // dots 1,3,6
/* 0X2A */ CMD_HWINRT,    // dots 1,3,5
/* 0X2B */ CMD_NOOP,      // dots 1,3,5,6
/* 0X2C */ CMD_DISPMD,    // dots 1,3,4
/* 0X2D */ CMD_PREFMENU,  // dots 1,3,4,6
/* 0X2E */ CMD_NOOP,      // dots 1,3,4,5
/* 0X2F */ CMD_NOOP,      // dots 1,3,4,5,6
/* 0X30 */ CMD_LNBEG,     // dots 2,3
/* 0X31 */ CMD_CUT_BEG,   // dots 2,3,6
/* 0X32 */ CMD_SIXDOTS,   // dots 2,3,5
/* 0X33 */ CMD_NOOP,      // dots 2,3,5,6
/* 0X34 */ CMD_SND,       // dots 2,3,4
/* 0X35 */ CMD_NOOP,      // dots 2,3,4,6
/* 0X36 */ CMD_NOOP,      // dots 2,3,4,5
/* 0X37 */ CMD_NOOP,      // dots 2,3,4,5,6
/* 0X38 */ CMD_TOP,       // dots 1,2,3
/* 0X39 */ CMD_NOOP,      // dots 1,2,3,6
/* 0X3A */ CMD_PREFLOAD,  // dots 1,2,3,5
/* 0X3B */ CMD_NOOP,      // dots 1,2,3,5,6
/* 0X3C */ CMD_PASTE,     // dots 1,2,3,4
/* 0X3D */ CMD_NOOP,      // dots 1,2,3,4,6
/* 0X3E */ CMD_NOOP,      // dots 1,2,3,4,5
/* 0X3F */ CMD_PREFSAVE,  // dots 1,2,3,4,5,6
/* 0X40 */ CMD_NOOP,      // unexpected
/* 0X41 */ CMD_NOOP,      // unexpected
/* 0X42 */ CMD_NOOP,      // unexpected
/* 0X43 */ CMD_NOOP,      // unexpected
/* 0X44 */ CMD_NOOP,      // unexpected
/* 0X45 */ CMD_NOOP,      // unexpected
/* 0X46 */ CMD_NOOP,      // unexpected
/* 0X47 */ CMD_NOOP,      // unexpected
/* 0X48 */ CMD_NOOP,      // unexpected
/* 0X49 */ CMD_NOOP,      // unexpected
/* 0X4A */ CMD_NOOP,      // unexpected
/* 0X4B */ CMD_NOOP,      // unexpected
/* 0X4C */ CMD_NOOP,      // unexpected
/* 0X4D */ CMD_NOOP,      // unexpected
/* 0X4E */ CMD_NOOP,      // unexpected
/* 0X4F */ CMD_NOOP,      // unexpected
/* 0X50 */ CMD_NOOP,      // unexpected
/* 0X51 */ CMD_NOOP,      // unexpected
/* 0X52 */ CMD_NOOP,      // unexpected
/* 0X53 */ CMD_NOOP,      // unexpected
/* 0X54 */ CMD_NOOP,      // unexpected
/* 0X55 */ CMD_NOOP,      // unexpected
/* 0X56 */ CMD_NOOP,      // unexpected
/* 0X57 */ CMD_NOOP,      // unexpected
/* 0X58 */ CMD_NOOP,      // unexpected
/* 0X59 */ CMD_NOOP,      // unexpected
/* 0X5A */ CMD_NOOP,      // unexpected
/* 0X5B */ CMD_NOOP,      // unexpected
/* 0X5C */ CMD_NOOP,      // unexpected
/* 0X5D */ CMD_NOOP,      // unexpected
/* 0X5E */ CMD_NOOP,      // unexpected
/* 0X5F */ CMD_NOOP,      // unexpected
/* 0X60 */ CMD_NOOP,      // no thumb keys
/* 0X61 */ CMD_FWINLT,    // thumb keys A
/* 0X62 */ CMD_LNUP,      // thumb keys B
/* 0X63 */ CMD_TOP_LEFT,  // thumb keys A,B
/* 0X64 */ CMD_CSRTRK,    // thumb keys C
/* 0X65 */ CMD_LNBEG,     // thumb keys A,C
/* 0X66 */ CMD_TOP,       // thumb keys B,C
/* 0X67 */ CMD_NOOP,      // thumb keys A,B,C
/* 0X68 */ CMD_LNDN,      // thumb keys D
/* 0X69 */ CMD_HWINLT,    // thumb keys A,D
/* 0X6A */ CMD_MUTE,      // thumb keys B,D
/* 0X6B */ CMD_NOOP,      // thumb keys A,B,D
/* 0X6C */ CMD_BOT,       // thumb keys C,D
/* 0X6D */ CMD_NOOP,      // thumb keys A,C,D
/* 0X6E */ CMD_NOOP,      // thumb keys B,C,D
/* 0X6F */ CMD_NOOP,      // thumb keys A,B,C,D
/* 0X70 */ CMD_FWINRT,    // thumb keys E
/* 0X71 */ CMD_SAY,       // thumb keys A,E
/* 0X72 */ CMD_HWINRT,    // thumb keys B,E
/* 0X73 */ CMD_NOOP,      // thumb keys A,B,E
/* 0X74 */ CMD_LNEND,     // thumb keys C,E
/* 0X75 */ CMD_NOOP,      // thumb keys A,C,E
/* 0X76 */ CMD_NOOP,      // thumb keys B,C,E
/* 0X77 */ CMD_NOOP,      // thumb keys A,B,C,E
/* 0X78 */ CMD_BOT_LEFT,  // thumb keys D,E
/* 0X79 */ CMD_NOOP,      // thumb keys A,D,E
/* 0X7A */ CMD_NOOP,      // thumb keys B,D,E
/* 0X7B */ CMD_NOOP,      // thumb keys A,B,D,E
/* 0X7C */ CMD_NOOP,      // thumb keys C,D,E
/* 0X7D */ CMD_NOOP,      // thumb keys A,C,D,E
/* 0X7E */ CMD_NOOP,      // thumb keys B,C,D,E
/* 0X7F */ CMD_NOOP,      // thumb keys A,B,C,D,E
/* 0X80 */ CMD_ATTRVIS,   // status key 1
/* 0X81 */ CMD_ATTRBLINK, // status key 2
/* 0X82 */ CMD_PREFMENU,  // status key 3
/* 0X83 */ CMD_PREFLOAD,  // status key 4
/* 0X84 */ CMD_FREEZE,    // status key 5
/* 0X85 */ CMD_HELP,      // status key 6
/* 0X86 */ 0X80,          // routing key 1
/* 0X87 */ 0X81,          // routing key 2
/* 0X88 */ 0X82,          // routing key 3
/* 0X89 */ 0X83,          // routing key 4
/* 0X8A */ 0X84,          // routing key 5
/* 0X8B */ 0X85,          // routing key 6
/* 0X8C */ 0X86,          // routing key 7
/* 0X8D */ 0X87,          // routing key 8
/* 0X8E */ 0X88,          // routing key 9
/* 0X8F */ 0X89,          // routing key 10
/* 0X90 */ 0X8A,          // routing key 11
/* 0X91 */ 0X8B,          // routing key 12
/* 0X92 */ 0X8C,          // routing key 13
/* 0X93 */ 0X8D,          // routing key 14
/* 0X94 */ 0X8E,          // routing key 15
/* 0X95 */ 0X8F,          // routing key 16
/* 0X96 */ 0X90,          // routing key 17
/* 0X97 */ 0X91,          // routing key 18
/* 0X98 */ 0X92,          // routing key 19
/* 0X99 */ 0X93,          // routing key 20
/* 0X9A */ 0X94,          // routing key 21
/* 0X9B */ 0X95,          // routing key 22
/* 0X9C */ 0X96,          // routing key 23
/* 0X9D */ 0X97,          // routing key 24
/* 0X9E */ 0X98,          // routing key 25
/* 0X9F */ 0X99,          // routing key 26
/* 0XA0 */ 0X9A,          // routing key 27
/* 0XA1 */ 0X9B,          // routing key 28
/* 0XA2 */ 0X9C,          // routing key 29
/* 0XA3 */ 0X9D,          // routing key 30
/* 0XA4 */ 0X9E,          // routing key 31
/* 0XA5 */ 0X9F,          // routing key 32
/* 0XA6 */ 0XA0,          // routing key 33
/* 0XA7 */ 0XA1,          // routing key 34
/* 0XA8 */ 0XA2,          // routing key 35
/* 0XA9 */ 0XA3,          // routing key 36
/* 0XAA */ 0XA4,          // routing key 37
/* 0XAB */ 0XA5,          // routing key 38
/* 0XAC */ 0XA6,          // routing key 39
/* 0XAD */ 0XA7,          // routing key 40
/* 0XAE */ 0XA8,          // routing key 41
/* 0XAF */ 0XA9,          // routing key 42
/* 0XB0 */ 0XAA,          // routing key 43
/* 0XB1 */ 0XAB,          // routing key 44
/* 0XB2 */ 0XAC,          // routing key 45
/* 0XB3 */ 0XAD,          // routing key 46
/* 0XB4 */ 0XAE,          // routing key 47
/* 0XB5 */ 0XAF,          // routing key 48
/* 0XB6 */ 0XB0,          // routing key 49
/* 0XB7 */ 0XB1,          // routing key 50
/* 0XB8 */ 0XB2,          // routing key 51
/* 0XB9 */ 0XB3,          // routing key 52
/* 0XBA */ 0XB4,          // routing key 53
/* 0XBB */ 0XB5,          // routing key 54
/* 0XBC */ 0XB6,          // routing key 55
/* 0XBD */ 0XB7,          // routing key 56
/* 0XBE */ 0XB8,          // routing key 57
/* 0XBF */ 0XB9,          // routing key 58
/* 0XC0 */ 0XBA,          // routing key 59
/* 0XC1 */ 0XBB,          // routing key 60
/* 0XC2 */ 0XBC,          // routing key 61
/* 0XC3 */ 0XBD,          // routing key 62
/* 0XC4 */ 0XBE,          // routing key 63
/* 0XC5 */ 0XBF,          // routing key 64
/* 0XC6 */ 0XC0,          // routing key 65
/* 0XC7 */ 0XC1,          // routing key 66
/* 0XC8 */ 0XC2,          // routing key 67
/* 0XC9 */ 0XC3,          // routing key 68
/* 0XCA */ 0XC4,          // routing key 69
/* 0XCB */ 0XC5,          // routing key 70
/* 0XCC */ 0XC6,          // routing key 71
/* 0XCD */ 0XC7,          // routing key 72
/* 0XCE */ 0XC8,          // routing key 73
/* 0XCF */ 0XC9,          // routing key 74
/* 0XD0 */ 0XCA,          // routing key 75
/* 0XD1 */ 0XCB,          // routing key 76
/* 0XD2 */ 0XCC,          // routing key 77
/* 0XD3 */ 0XCD,          // routing key 78
/* 0XD4 */ 0XCE,          // routing key 79
/* 0XD5 */ 0XCF,          // routing key 80
/* 0XD6 */ 0XD0,          // routing key 81
/* 0XD7 */ 0XD1,          // routing key 82
/* 0XD8 */ 0XD2,          // routing key 83
/* 0XD9 */ 0XD3,          // routing key 84
/* 0XDA */ 0XD4,          // routing key 85
/* 0XDB */ 0XD5,          // routing key 86
/* 0XDC */ 0XD6,          // routing key 87
/* 0XDD */ 0XD7,          // routing key 88
/* 0XDE */ 0XD8,          // routing key 89
/* 0XDF */ 0XD9,          // routing key 90
/* 0XE0 */ 0XDA,          // routing key 91
/* 0XE1 */ 0XDB,          // routing key 92
/* 0XE2 */ 0XDC,          // routing key 93
/* 0XE3 */ 0XDD,          // routing key 94
/* 0XE4 */ 0XDE,          // routing key 95
/* 0XE5 */ 0XDF,          // routing key 96
/* 0XE6 */ 0XE0,          // routing key 97
/* 0XE7 */ 0XE1,          // routing key 98
/* 0XE8 */ 0XE2,          // routing key 99
/* 0XE9 */ 0XE3,          // routing key 100
/* 0XEA */ 0XE4,          // routing key 101
/* 0XEB */ 0XE5,          // routing key 102
/* 0XEC */ 0XE6,          // routing key 103
/* 0XED */ 0XE7,          // routing key 104
/* 0XEE */ 0XE8,          // routing key 105
/* 0XEF */ 0XE9,          // routing key 106
/* 0XF0 */ 0XEA,          // routing key 107
/* 0XF1 */ 0XEB,          // routing key 108
/* 0XF2 */ 0XEC,          // routing key 109
/* 0XF3 */ 0XED,          // routing key 110
/* 0XF4 */ 0XEE,          // routing key 111
/* 0XF5 */ 0XEF,          // routing key 112
/* 0XF6 */ 0XF0,          // routing key 113
/* 0XF7 */ 0XF1,          // routing key 114
/* 0XF8 */ 0XF2,          // routing key 115
/* 0XF9 */ 0XF3,          // routing key 116
/* 0XFA */ 0XF4,          // routing key 117
/* 0XFB */ 0XF5,          // routing key 118
/* 0XFC */ 0XF6,          // routing key 119
/* 0XFD */ 0XF7,          // routing key 120
/* 0XFE */ 0XF8,          // routing key 121
/* 0XFF */ 0XF9           // routing key 122
