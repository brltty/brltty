/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

/* 0X00 */ BRL_CMD_NOOP,                   /* no dots          */
/* 0X01 */ BRL_CMD_FWINRT,                 /* dot 6            */
/* 0X02 */ BRL_CMD_CSRVIS,                 /* dot 5            */
/* 0X03 */ BRL_CMD_LNEND,                  /* dots 5,6         */
/* 0X04 */ BRL_CMD_LNDN,                   /* dot 4            */
/* 0X05 */ BRL_CMD_CHRRT,                  /* dots 4,6         */
/* 0X06 */ BRL_CMD_WINDN,                  /* dots 4,5         */
/* 0X07 */ BRL_CMD_BOT,                    /* dots 4,5,6       */
/* 0X08 */ BRL_CMD_LNUP,                   /* dot 1            */
/* 0X09 */ BRL_CMD_MUTE,                   /* dots 1,6         */
/* 0X0A */ BRL_CMD_NOOP,                   /* dots 1,5         */
/* 0X0B */ BRL_CMD_SAY_LINE,               /* dots 1,5,6       */
/* 0X0C */ BRL_CMD_CSRTRK,                 /* dots 1,4         */
/* 0X0D */ BRL_CMD_CAPBLINK,               /* dots 1,4,6       */
/* 0X0E */ BRL_CMD_NOOP,                   /* dots 1,4,5       */
/* 0X0F */ BRL_CMD_NOOP,                   /* dots 1,4,5,6     */
/* 0X10 */ BRL_CMD_CSRBLINK,               /* dot 2            */
/* 0X11 */ BRL_CMD_NOOP,                   /* dots 2,6         */
/* 0X12 */ BRL_CMD_CSRSIZE,                /* dots 2,5         */
/* 0X13 */ BRL_CMD_NOOP,                   /* dots 2,5,6       */
/* 0X14 */ BRL_CMD_NOOP,                   /* dots 2,4         */
/* 0X15 */ BRL_CMD_HWINLT,                 /* dots 2,4,6       */
/* 0X16 */ BRL_CMD_SKPIDLNS,               /* dots 2,4,5       */
/* 0X17 */ BRL_CMD_SLIDEWIN,               /* dots 2,4,5,6     */
/* 0X18 */ BRL_CMD_WINUP,                  /* dots 1,2         */
/* 0X19 */ BRL_CMD_NOOP,                   /* dots 1,2,6       */
/* 0X1A */ BRL_CMD_HELP,                   /* dots 1,2,5       */
/* 0X1B */ BRL_CMD_NOOP,                   /* dots 1,2,5,6     */
/* 0X1C */ BRL_CMD_FREEZE,                 /* dots 1,2,4       */
/* 0X1D */ BRL_CMD_NOOP,                   /* dots 1,2,4,6     */
/* 0X1E */ BRL_BLK_ROUTE,                       /* dots 1,2,4,5     */
/* 0X1F */ BRL_CMD_NOOP,                   /* dots 1,2,4,5,6   */
/* 0X20 */ BRL_CMD_FWINLT,                 /* dot 3            */
/* 0X21 */ BRL_CMD_HOME,                   /* dots 3,6         */
/* 0X22 */ BRL_CMD_NOOP,                   /* dots 3,5         */
/* 0X23 */ BRL_BLK_CUTRECT + BRL_MSK_ARG - 1,   /* dots 3,5,6       */
/* 0X24 */ BRL_CMD_INFO,                   /* dots 3,4         */
/* 0X25 */ BRL_CMD_NOOP,                   /* dots 3,4,6       */
/* 0X26 */ BRL_CMD_NOOP,                   /* dots 3,4,5       */
/* 0X27 */ BRL_CMD_NOOP,                   /* dots 3,4,5,6     */
/* 0X28 */ BRL_CMD_CHRLT,                  /* dots 1,3         */
/* 0X29 */ BRL_CMD_NOOP,                   /* dots 1,3,6       */
/* 0X2A */ BRL_CMD_HWINRT,                 /* dots 1,3,5       */
/* 0X2B */ BRL_CMD_NOOP,                   /* dots 1,3,5,6     */
/* 0X2C */ BRL_CMD_DISPMD,                 /* dots 1,3,4       */
/* 0X2D */ BRL_CMD_PREFMENU,               /* dots 1,3,4,6     */
/* 0X2E */ BRL_CMD_NOOP,                   /* dots 1,3,4,5     */
/* 0X2F */ BRL_CMD_NOOP,                   /* dots 1,3,4,5,6   */
/* 0X30 */ BRL_CMD_LNBEG,                  /* dots 2,3         */
/* 0X31 */ BRL_BLK_CUTBEGIN,                    /* dots 2,3,6       */
/* 0X32 */ BRL_CMD_SIXDOTS,                /* dots 2,3,5       */
/* 0X33 */ BRL_CMD_NOOP,                   /* dots 2,3,5,6     */
/* 0X34 */ BRL_CMD_TUNES,                  /* dots 2,3,4       */
/* 0X35 */ BRL_CMD_NOOP,                   /* dots 2,3,4,6     */
/* 0X36 */ BRL_CMD_NOOP,                   /* dots 2,3,4,5     */
/* 0X37 */ BRL_CMD_NOOP,                   /* dots 2,3,4,5,6   */
/* 0X38 */ BRL_CMD_TOP,                    /* dots 1,2,3       */
/* 0X39 */ BRL_CMD_NOOP,                   /* dots 1,2,3,6     */
/* 0X3A */ BRL_CMD_PREFLOAD,               /* dots 1,2,3,5     */
/* 0X3B */ BRL_CMD_NOOP,                   /* dots 1,2,3,5,6   */
/* 0X3C */ BRL_CMD_PASTE,                  /* dots 1,2,3,4     */
/* 0X3D */ BRL_CMD_NOOP,                   /* dots 1,2,3,4,6   */
/* 0X3E */ BRL_CMD_NOOP,                   /* dots 1,2,3,4,5   */
/* 0X3F */ BRL_CMD_PREFSAVE,               /* dots 1,2,3,4,5,6 */
/* 0X40 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X41 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X42 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X43 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X44 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X45 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X46 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X47 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X48 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X49 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X4A */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X4B */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X4C */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X4D */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X4E */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X4F */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X50 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X51 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X52 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X53 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X54 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X55 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X56 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X57 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X58 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X59 */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X5A */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X5B */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X5C */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X5D */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X5E */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X5F */ BRL_CMD_NOOP,                   /* unexpected */
/* 0X60 */ BRL_CMD_NOOP,                   /* no thumb keys        */
/* 0X61 */ BRL_CMD_FWINLT,                 /* thumb key A          */
/* 0X62 */ BRL_CMD_LNUP,                   /* thumb key B          */
/* 0X63 */ BRL_CMD_TOP_LEFT,               /* thumb keys A,B       */
/* 0X64 */ BRL_CMD_CSRTRK,                 /* thumb key C          */
/* 0X65 */ BRL_CMD_LNBEG,                  /* thumb keys A,C       */
/* 0X66 */ BRL_CMD_TOP,                    /* thumb keys B,C       */
/* 0X67 */ BRL_CMD_NOOP,                   /* thumb keys A,B,C     */
/* 0X68 */ BRL_CMD_LNDN,                   /* thumb key D          */
/* 0X69 */ BRL_CMD_HWINLT,                 /* thumb keys A,D       */
/* 0X6A */ BRL_CMD_MUTE,                   /* thumb keys B,D       */
/* 0X6B */ BRL_CMD_NOOP,                   /* thumb keys A,B,D     */
/* 0X6C */ BRL_CMD_BOT,                    /* thumb keys C,D       */
/* 0X6D */ BRL_CMD_NOOP,                   /* thumb keys A,C,D     */
/* 0X6E */ BRL_CMD_NOOP,                   /* thumb keys B,C,D     */
/* 0X6F */ BRL_CMD_NOOP,                   /* thumb keys A,B,C,D   */
/* 0X70 */ BRL_CMD_FWINRT,                 /* thumb key E          */
/* 0X71 */ BRL_CMD_SAY_LINE,               /* thumb keys A,E       */
/* 0X72 */ BRL_CMD_HWINRT,                 /* thumb keys B,E       */
/* 0X73 */ BRL_CMD_NOOP,                   /* thumb keys A,B,E     */
/* 0X74 */ BRL_CMD_LNEND,                  /* thumb keys C,E       */
/* 0X75 */ BRL_CMD_NOOP,                   /* thumb keys A,C,E     */
/* 0X76 */ BRL_CMD_NOOP,                   /* thumb keys B,C,E     */
/* 0X77 */ BRL_CMD_NOOP,                   /* thumb keys A,B,C,E   */
/* 0X78 */ BRL_CMD_BOT_LEFT,               /* thumb keys D,E       */
/* 0X79 */ BRL_CMD_NOOP,                   /* thumb keys A,D,E     */
/* 0X7A */ BRL_CMD_NOOP,                   /* thumb keys B,D,E     */
/* 0X7B */ BRL_CMD_NOOP,                   /* thumb keys A,B,D,E   */
/* 0X7C */ BRL_CMD_NOOP,                   /* thumb keys C,D,E     */
/* 0X7D */ BRL_CMD_NOOP,                   /* thumb keys A,C,D,E   */
/* 0X7E */ BRL_CMD_NOOP,                   /* thumb keys B,C,D,E   */
/* 0X7F */ BRL_CMD_NOOP,                   /* thumb keys A,B,C,D,E */
/* 0X80 */ BRL_BLK_CUTBEGIN + BRL_MSK_ARG, /* status key 1 */
/* 0X81 */ BRL_BLK_CUTRECT + BRL_MSK_ARG,  /* status key 2 */
/* 0X82 */ BRL_CMD_PREFMENU,               /* status key 3 */
/* 0X83 */ BRL_CMD_PREFLOAD,               /* status key 4 */
/* 0X84 */ BRL_CMD_FREEZE,                 /* status key 5 */
/* 0X85 */ BRL_CMD_HELP,                   /* status key 6 */
/* 0X86 */ BRL_BLK_ROUTE +   0,            /* routing key   1 */
/* 0X87 */ BRL_BLK_ROUTE +   1,            /* routing key   2 */
/* 0X88 */ BRL_BLK_ROUTE +   2,            /* routing key   3 */
/* 0X89 */ BRL_BLK_ROUTE +   3,            /* routing key   4 */
/* 0X8A */ BRL_BLK_ROUTE +   4,            /* routing key   5 */
/* 0X8B */ BRL_BLK_ROUTE +   5,            /* routing key   6 */
/* 0X8C */ BRL_BLK_ROUTE +   6,            /* routing key   7 */
/* 0X8D */ BRL_BLK_ROUTE +   7,            /* routing key   8 */
/* 0X8E */ BRL_BLK_ROUTE +   8,            /* routing key   9 */
/* 0X8F */ BRL_BLK_ROUTE +   9,            /* routing key  10 */
/* 0X90 */ BRL_BLK_ROUTE +  10,            /* routing key  11 */
/* 0X91 */ BRL_BLK_ROUTE +  11,            /* routing key  12 */
/* 0X92 */ BRL_BLK_ROUTE +  12,            /* routing key  13 */
/* 0X93 */ BRL_BLK_ROUTE +  13,            /* routing key  14 */
/* 0X94 */ BRL_BLK_ROUTE +  14,            /* routing key  15 */
/* 0X95 */ BRL_BLK_ROUTE +  15,            /* routing key  16 */
/* 0X96 */ BRL_BLK_ROUTE +  16,            /* routing key  17 */
/* 0X97 */ BRL_BLK_ROUTE +  17,            /* routing key  18 */
/* 0X98 */ BRL_BLK_ROUTE +  18,            /* routing key  19 */
/* 0X99 */ BRL_BLK_ROUTE +  19,            /* routing key  20 */
/* 0X9A */ BRL_BLK_ROUTE +  20,            /* routing key  21 */
/* 0X9B */ BRL_BLK_ROUTE +  21,            /* routing key  22 */
/* 0X9C */ BRL_BLK_ROUTE +  22,            /* routing key  23 */
/* 0X9D */ BRL_BLK_ROUTE +  23,            /* routing key  24 */
/* 0X9E */ BRL_BLK_ROUTE +  24,            /* routing key  25 */
/* 0X9F */ BRL_BLK_ROUTE +  25,            /* routing key  26 */
/* 0XA0 */ BRL_BLK_ROUTE +  26,            /* routing key  27 */
/* 0XA1 */ BRL_BLK_ROUTE +  27,            /* routing key  28 */
/* 0XA2 */ BRL_BLK_ROUTE +  28,            /* routing key  29 */
/* 0XA3 */ BRL_BLK_ROUTE +  29,            /* routing key  30 */
/* 0XA4 */ BRL_BLK_ROUTE +  30,            /* routing key  31 */
/* 0XA5 */ BRL_BLK_ROUTE +  31,            /* routing key  32 */
/* 0XA6 */ BRL_BLK_ROUTE +  32,            /* routing key  33 */
/* 0XA7 */ BRL_BLK_ROUTE +  33,            /* routing key  34 */
/* 0XA8 */ BRL_BLK_ROUTE +  34,            /* routing key  35 */
/* 0XA9 */ BRL_BLK_ROUTE +  35,            /* routing key  36 */
/* 0XAA */ BRL_BLK_ROUTE +  36,            /* routing key  37 */
/* 0XAB */ BRL_BLK_ROUTE +  37,            /* routing key  38 */
/* 0XAC */ BRL_BLK_ROUTE +  38,            /* routing key  39 */
/* 0XAD */ BRL_BLK_ROUTE +  39,            /* routing key  40 */
/* 0XAE */ BRL_BLK_ROUTE +  40,            /* routing key  41 */
/* 0XAF */ BRL_BLK_ROUTE +  41,            /* routing key  42 */
/* 0XB0 */ BRL_BLK_ROUTE +  42,            /* routing key  43 */
/* 0XB1 */ BRL_BLK_ROUTE +  43,            /* routing key  44 */
/* 0XB2 */ BRL_BLK_ROUTE +  44,            /* routing key  45 */
/* 0XB3 */ BRL_BLK_ROUTE +  45,            /* routing key  46 */
/* 0XB4 */ BRL_BLK_ROUTE +  46,            /* routing key  47 */
/* 0XB5 */ BRL_BLK_ROUTE +  47,            /* routing key  48 */
/* 0XB6 */ BRL_BLK_ROUTE +  48,            /* routing key  49 */
/* 0XB7 */ BRL_BLK_ROUTE +  49,            /* routing key  50 */
/* 0XB8 */ BRL_BLK_ROUTE +  50,            /* routing key  51 */
/* 0XB9 */ BRL_BLK_ROUTE +  51,            /* routing key  52 */
/* 0XBA */ BRL_BLK_ROUTE +  52,            /* routing key  53 */
/* 0XBB */ BRL_BLK_ROUTE +  53,            /* routing key  54 */
/* 0XBC */ BRL_BLK_ROUTE +  54,            /* routing key  55 */
/* 0XBD */ BRL_BLK_ROUTE +  55,            /* routing key  56 */
/* 0XBE */ BRL_BLK_ROUTE +  56,            /* routing key  57 */
/* 0XBF */ BRL_BLK_ROUTE +  57,            /* routing key  58 */
/* 0XC0 */ BRL_BLK_ROUTE +  58,            /* routing key  59 */
/* 0XC1 */ BRL_BLK_ROUTE +  59,            /* routing key  60 */
/* 0XC2 */ BRL_BLK_ROUTE +  60,            /* routing key  61 */
/* 0XC3 */ BRL_BLK_ROUTE +  61,            /* routing key  62 */
/* 0XC4 */ BRL_BLK_ROUTE +  62,            /* routing key  63 */
/* 0XC5 */ BRL_BLK_ROUTE +  63,            /* routing key  64 */
/* 0XC6 */ BRL_BLK_ROUTE +  64,            /* routing key  65 */
/* 0XC7 */ BRL_BLK_ROUTE +  65,            /* routing key  66 */
/* 0XC8 */ BRL_BLK_ROUTE +  66,            /* routing key  67 */
/* 0XC9 */ BRL_BLK_ROUTE +  67,            /* routing key  68 */
/* 0XCA */ BRL_BLK_ROUTE +  68,            /* routing key  69 */
/* 0XCB */ BRL_BLK_ROUTE +  69,            /* routing key  70 */
/* 0XCC */ BRL_BLK_ROUTE +  70,            /* routing key  71 */
/* 0XCD */ BRL_BLK_ROUTE +  71,            /* routing key  72 */
/* 0XCE */ BRL_BLK_ROUTE +  72,            /* routing key  73 */
/* 0XCF */ BRL_BLK_ROUTE +  73,            /* routing key  74 */
/* 0XD0 */ BRL_BLK_ROUTE +  74,            /* routing key  75 */
/* 0XD1 */ BRL_BLK_ROUTE +  75,            /* routing key  76 */
/* 0XD2 */ BRL_BLK_ROUTE +  76,            /* routing key  77 */
/* 0XD3 */ BRL_BLK_ROUTE +  77,            /* routing key  78 */
/* 0XD4 */ BRL_BLK_ROUTE +  78,            /* routing key  79 */
/* 0XD5 */ BRL_BLK_ROUTE +  79,            /* routing key  80 */
/* 0XD6 */ BRL_BLK_ROUTE +  80,            /* routing key  81 */
/* 0XD7 */ BRL_BLK_ROUTE +  81,            /* routing key  82 */
/* 0XD8 */ BRL_BLK_ROUTE +  82,            /* routing key  83 */
/* 0XD9 */ BRL_BLK_ROUTE +  83,            /* routing key  84 */
/* 0XDA */ BRL_BLK_ROUTE +  84,            /* routing key  85 */
/* 0XDB */ BRL_BLK_ROUTE +  85,            /* routing key  86 */
/* 0XDC */ BRL_BLK_ROUTE +  86,            /* routing key  87 */
/* 0XDD */ BRL_BLK_ROUTE +  87,            /* routing key  88 */
/* 0XDE */ BRL_BLK_ROUTE +  88,            /* routing key  89 */
/* 0XDF */ BRL_BLK_ROUTE +  89,            /* routing key  90 */
/* 0XE0 */ BRL_BLK_ROUTE +  90,            /* routing key  91 */
/* 0XE1 */ BRL_BLK_ROUTE +  91,            /* routing key  92 */
/* 0XE2 */ BRL_BLK_ROUTE +  92,            /* routing key  93 */
/* 0XE3 */ BRL_BLK_ROUTE +  93,            /* routing key  94 */
/* 0XE4 */ BRL_BLK_ROUTE +  94,            /* routing key  95 */
/* 0XE5 */ BRL_BLK_ROUTE +  95,            /* routing key  96 */
/* 0XE6 */ BRL_BLK_ROUTE +  96,            /* routing key  97 */
/* 0XE7 */ BRL_BLK_ROUTE +  97,            /* routing key  98 */
/* 0XE8 */ BRL_BLK_ROUTE +  98,            /* routing key  99 */
/* 0XE9 */ BRL_BLK_ROUTE +  99,            /* routing key 100 */
/* 0XEA */ BRL_BLK_ROUTE + 100,            /* routing key 101 */
/* 0XEB */ BRL_BLK_ROUTE + 101,            /* routing key 102 */
/* 0XEC */ BRL_BLK_ROUTE + 102,            /* routing key 103 */
/* 0XED */ BRL_BLK_ROUTE + 103,            /* routing key 104 */
/* 0XEE */ BRL_BLK_ROUTE + 104,            /* routing key 105 */
/* 0XEF */ BRL_BLK_ROUTE + 105,            /* routing key 106 */
/* 0XF0 */ BRL_BLK_ROUTE + 106,            /* routing key 107 */
/* 0XF1 */ BRL_BLK_ROUTE + 107,            /* routing key 108 */
/* 0XF2 */ BRL_BLK_ROUTE + 108,            /* routing key 109 */
/* 0XF3 */ BRL_BLK_ROUTE + 109,            /* routing key 110 */
/* 0XF4 */ BRL_BLK_ROUTE + 110,            /* routing key 111 */
/* 0XF5 */ BRL_BLK_ROUTE + 111,            /* routing key 112 */
/* 0XF6 */ BRL_BLK_ROUTE + 112,            /* routing key 113 */
/* 0XF7 */ BRL_BLK_ROUTE + 113,            /* routing key 114 */
/* 0XF8 */ BRL_BLK_ROUTE + 114,            /* routing key 115 */
/* 0XF9 */ BRL_BLK_ROUTE + 115,            /* routing key 116 */
/* 0XFA */ BRL_BLK_ROUTE + 116,            /* routing key 117 */
/* 0XFB */ BRL_BLK_ROUTE + 117,            /* routing key 118 */
/* 0XFC */ BRL_BLK_ROUTE + 118,            /* routing key 119 */
/* 0XFD */ BRL_BLK_ROUTE + 119,            /* routing key 120 */
/* 0XFE */ BRL_BLK_ROUTE + 120,            /* routing key 121 */
/* 0XFF */ BRL_BLK_ROUTE + 121             /* routing key 122 */
