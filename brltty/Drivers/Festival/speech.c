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

/* BRLTTY speech driver for the Festival text to speech engine.
 * Written by: Nikhil Nair <nn201@cus.cam.ac.uk>
 * Maintained by: Dave Mielke <dave@mielke.cc>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "Programs/misc.h"

typedef enum {
  PARM_COMMAND,
  PARM_NAME
} DriverParameter;
#define SPKPARMS "command", "name"

#define SPK_HAVE_RATE
#include "Programs/spk_driver.h"
#include "speech.h"		/* for speech definitions */

static const char *wordTable[] = {
  /*   0 00 ^@ NUL             */ " ",
  /*   1 01 ^A SOH             */ " ",
  /*   2 02 ^B STX             */ " ",
  /*   3 03 ^C ETX             */ " ",
  /*   4 04 ^D EOT             */ " ",
  /*   5 05 ^E ENQ             */ " ",
  /*   6 06 ^F ACK             */ " ",
  /*   7 07 ^G BEL             */ " ",
  /*   8 08 ^H BS              */ " ",
  /*   9 09 ^I HT              */ " ",
  /*  10 0A ^J LF              */ " ",
  /*  11 0B ^K VT              */ " ",
  /*  12 0C ^L FF              */ " ",
  /*  13 0D ^M CR              */ " ",
  /*  14 0E ^N SO              */ " ",
  /*  15 0F ^O SI              */ " ",
  /*  16 10 ^P DLE             */ " ",
  /*  17 11 ^Q DC1             */ " ",
  /*  18 12 ^R DC2             */ " ",
  /*  19 13 ^S DC3             */ " ",
  /*  20 14 ^T DC4             */ " ",
  /*  21 15 ^U NAK             */ " ",
  /*  22 16 ^V SYN             */ " ",
  /*  23 17 ^W ETB             */ " ",
  /*  24 18 ^X CAN             */ " ",
  /*  25 19 ^Y EM              */ " ",
  /*  26 1A ^Z SUB             */ " ",
  /*  27 1B ^[ ESC             */ " ",
  /*  28 1C ^\ FS              */ " ",
  /*  29 1D ^] GS              */ " ",
  /*  30 1E ^^ RS              */ " ",
  /*  31 1F ^_ US              */ " ",
  /*  32 20    space           */ NULL,
  /*  33 21  ! exclamation     */ " exclamation ",
  /*  34 22  " quotedouble     */ " quote ",
  /*  35 23  # number          */ " hash ",
  /*  36 24  $ dollar          */ " dollar ",
  /*  37 25  % percent         */ " percent ",
  /*  38 26  & ampersand       */ " ampersand ",
  /*  39 27  ' quoteright      */ " apostrophe ",
  /*  40 28  ( parenleft       */ " left-paren ",
  /*  41 29  ) parenright      */ " right-paren ",
  /*  42 2A  * asterisk        */ " star ",
  /*  43 2B  + plus            */ " plus ",
  /*  44 2C  , comma           */ " comma ",
  /*  45 2D  - minus           */ " dash ",
  /*  46 2E  . period          */ " dot ",
  /*  47 2F  / slash           */ " slash ",
  /*  48 30  0 zero            */ NULL,
  /*  49 31  1 one             */ NULL,
  /*  50 32  2 two             */ NULL,
  /*  51 33  3 three           */ NULL,
  /*  52 34  4 four            */ NULL,
  /*  53 35  5 five            */ NULL,
  /*  54 36  6 six             */ NULL,
  /*  55 37  7 seven           */ NULL,
  /*  56 38  8 eight           */ NULL,
  /*  57 39  9 nine            */ NULL,
  /*  58 3A  : colon           */ " colon ",
  /*  59 3B  ; semicolon       */ " semicolon ",
  /*  60 3C  < less            */ " less ",
  /*  61 3D  = equal           */ " equals ",
  /*  62 3E  > greater         */ " greater ",
  /*  63 3F  ? question        */ " question ",
  /*  64 40  @ at              */ " at ",
  /*  65 41  A A               */ NULL,
  /*  66 42  B B               */ NULL,
  /*  67 43  C C               */ NULL,
  /*  68 44  D D               */ NULL,
  /*  69 45  E E               */ NULL,
  /*  70 46  F F               */ NULL,
  /*  71 47  G G               */ NULL,
  /*  72 48  H H               */ NULL,
  /*  73 49  I I               */ NULL,
  /*  74 4A  J J               */ NULL,
  /*  75 4B  K K               */ NULL,
  /*  76 4C  L L               */ NULL,
  /*  77 4D  M M               */ NULL,
  /*  78 4E  N N               */ NULL,
  /*  79 4F  O O               */ NULL,
  /*  80 50  P P               */ NULL,
  /*  81 51  Q Q               */ NULL,
  /*  82 52  R R               */ NULL,
  /*  83 53  S S               */ NULL,
  /*  84 54  T T               */ NULL,
  /*  85 55  U U               */ NULL,
  /*  86 56  V V               */ NULL,
  /*  87 57  W W               */ NULL,
  /*  88 58  X X               */ NULL,
  /*  89 59  Y Y               */ NULL,
  /*  90 5A  Z Z               */ NULL,
  /*  91 5B  [ bracketleft     */ " left-bracket ",
  /*  92 5C  \ backslash       */ " backslash ",
  /*  93 5D  ] bracketright    */ " right-bracket ",
  /*  94 5E  ^ asciicircumflex */ " circumflex ",
  /*  95 5F  _ underscore      */ " underscore ",
  /*  96 60  ` quoteleft       */ " grave ",
  /*  97 61  a a               */ NULL,
  /*  98 62  b b               */ NULL,
  /*  99 63  c c               */ NULL,
  /* 100 64  d d               */ NULL,
  /* 101 65  e e               */ NULL,
  /* 102 66  f f               */ NULL,
  /* 103 67  g g               */ NULL,
  /* 104 68  h h               */ NULL,
  /* 105 69  i i               */ NULL,
  /* 106 6A  j j               */ NULL,
  /* 107 6B  k k               */ NULL,
  /* 108 6C  l l               */ NULL,
  /* 109 6D  m m               */ NULL,
  /* 110 6E  n n               */ NULL,
  /* 111 6F  o o               */ NULL,
  /* 112 70  p p               */ NULL,
  /* 113 71  q q               */ NULL,
  /* 114 72  r r               */ NULL,
  /* 115 73  s s               */ NULL,
  /* 116 74  t t               */ NULL,
  /* 117 75  u u               */ NULL,
  /* 118 76  v v               */ NULL,
  /* 119 77  w w               */ NULL,
  /* 120 78  x x               */ NULL,
  /* 121 79  y y               */ NULL,
  /* 122 7A  z z               */ NULL,
  /* 123 7B  { braceleft       */ " left-brace ",
  /* 124 7C  | barsolid        */ " bar ",
  /* 125 7D  } braceright      */ " right-brace ",
  /* 126 7E  ~ asciitilde      */ " tilde ",
  /* 127 7F ^? DEL             */ " ",
  /* 128 80 ~@ <80>            */ " ",
  /* 129 81 ~A <81>            */ " ",
  /* 130 82 ~B BPH             */ " ",
  /* 131 83 ~C NBH             */ " ",
  /* 132 84 ~D <84>            */ " ",
  /* 133 85 ~E NL              */ " ",
  /* 134 86 ~F SSA             */ " ",
  /* 135 87 ~G ESA             */ " ",
  /* 136 88 ~H CTS             */ " ",
  /* 137 89 ~I CTJ             */ " ",
  /* 138 8A ~J LTS             */ " ",
  /* 139 8B ~K PLD             */ " ",
  /* 140 8C ~L PLU             */ " ",
  /* 141 8D ~M RLF             */ " ",
  /* 142 8E ~N SS2             */ " ",
  /* 143 8F ~O SS3             */ " ",
  /* 144 90 ~P DCS             */ " ",
  /* 145 91 ~Q PU1             */ " ",
  /* 146 92 ~R PU2             */ " ",
  /* 147 93 ~S STS             */ " ",
  /* 148 94 ~T CC              */ " ",
  /* 149 95 ~U MW              */ " ",
  /* 150 96 ~V SGA             */ " ",
  /* 151 97 ~W EGA             */ " ",
  /* 152 98 ~X SS              */ " ",
  /* 153 99 ~Y <99>            */ " ",
  /* 154 9A ~Z SCI             */ " ",
  /* 155 9B ~[ CSI             */ " ",
  /* 156 9C ~\ ST              */ " ",
  /* 157 9D ~] OSC             */ " ",
  /* 158 9E ~^ PM              */ " ",
  /* 159 9F ~_ APC             */ " ",
  /* 160 A0 ~  space           */ NULL,
  /* 161 A1  ¡ exclamdown      */ NULL,
  /* 162 A2  ¢ cent            */ NULL,
  /* 163 A3  £ sterling        */ NULL,
  /* 164 A4  ¤ currency        */ NULL,
  /* 165 A5  ¥ yen             */ NULL,
  /* 166 A6  ¦ brokenbar       */ NULL,
  /* 167 A7  § section         */ NULL,
  /* 168 A8  ¨ dieresis        */ NULL,
  /* 169 A9  © copyright       */ NULL,
  /* 170 AA  ª ordfeminine     */ NULL,
  /* 171 AB  « guillemotleft   */ NULL,
  /* 172 AC  ¬ logicalnot      */ NULL,
  /* 173 AD  ­ hyphen          */ NULL,
  /* 174 AE  ® registered      */ NULL,
  /* 175 AF  ¯ macron          */ NULL,
  /* 176 B0  ° degree          */ NULL,
  /* 177 B1  ± plusminus       */ NULL,
  /* 178 B2  ² twosuperior     */ NULL,
  /* 179 B3  ³ threesuperior   */ NULL,
  /* 180 B4  ´ acute           */ NULL,
  /* 181 B5  µ mu              */ NULL,
  /* 182 B6  ¶ paragraph       */ NULL,
  /* 183 B7  · periodcentered  */ NULL,
  /* 184 B8  ¸ cedilla         */ NULL,
  /* 185 B9  ¹ onesuperior     */ NULL,
  /* 186 BA  º ordmasculine    */ NULL,
  /* 187 BB  » guillemotright  */ NULL,
  /* 188 BC  ¼ onequarter      */ NULL,
  /* 189 BD  ½ onehalf         */ NULL,
  /* 190 BE  ¾ threequarters   */ NULL,
  /* 191 BF  ¿ questiondown    */ NULL,
  /* 192 C0  À Agrave          */ NULL,
  /* 193 C1  Á Aacute          */ NULL,
  /* 194 C2  Â Acircumflex     */ NULL,
  /* 195 C3  Ã Atilde          */ NULL,
  /* 196 C4  Ä Adieresis       */ NULL,
  /* 197 C5  Å Aring           */ NULL,
  /* 198 C6  Æ AE              */ NULL,
  /* 199 C7  Ç Ccedilla        */ NULL,
  /* 200 C8  È Egrave          */ NULL,
  /* 201 C9  É Eacute          */ NULL,
  /* 202 CA  Ê Ecircumflex     */ NULL,
  /* 203 CB  Ë Edieresis       */ NULL,
  /* 204 CC  Ì Igrave          */ NULL,
  /* 205 CD  Í Iacute          */ NULL,
  /* 206 CE  Î Icircumflex     */ NULL,
  /* 207 CF  Ï Idieresis       */ NULL,
  /* 208 D0  Ð Eth             */ NULL,
  /* 209 D1  Ñ Ntilde          */ NULL,
  /* 210 D2  Ò Ograve          */ NULL,
  /* 211 D3  Ó Oacute          */ NULL,
  /* 212 D4  Ô Ocircumflex     */ NULL,
  /* 213 D5  Õ Otilde          */ NULL,
  /* 214 D6  Ö Odieresis       */ NULL,
  /* 215 D7  × multiply        */ NULL,
  /* 216 D8  Ø Oslash          */ NULL,
  /* 217 D9  Ù Ugrave          */ NULL,
  /* 218 DA  Ú Uacute          */ NULL,
  /* 219 DB  Û Ucircumflex     */ NULL,
  /* 220 DC  Ü Udieresis       */ NULL,
  /* 221 DD  Ý Yacute          */ NULL,
  /* 222 DE  Þ Thorn           */ NULL,
  /* 223 DF  ß germandbls      */ NULL,
  /* 224 E0  à agrave          */ NULL,
  /* 225 E1  á aacute          */ NULL,
  /* 226 E2  â acircumflex     */ NULL,
  /* 227 E3  ã atilde          */ NULL,
  /* 228 E4  ä adieresis       */ NULL,
  /* 229 E5  å aring           */ NULL,
  /* 230 E6  æ ae              */ NULL,
  /* 231 E7  ç ccedilla        */ NULL,
  /* 232 E8  è egrave          */ NULL,
  /* 233 E9  é eacute          */ NULL,
  /* 234 EA  ê ecircumflex     */ NULL,
  /* 235 EB  ë edieresis       */ NULL,
  /* 236 EC  ì igrave          */ NULL,
  /* 237 ED  í iacute          */ NULL,
  /* 238 EE  î icircumflex     */ NULL,
  /* 239 EF  ï idieresis       */ NULL,
  /* 240 F0  ð eth             */ NULL,
  /* 241 F1  ñ ntilde          */ NULL,
  /* 242 F2  ò ograve          */ NULL,
  /* 243 F3  ó oacute          */ NULL,
  /* 244 F4  ô ocircumflex     */ NULL,
  /* 245 F5  õ otilde          */ NULL,
  /* 246 F6  ö odieresis       */ NULL,
  /* 247 F7  ÷ divide          */ NULL,
  /* 248 F8  ø oslash          */ NULL,
  /* 249 F9  ù ugrave          */ NULL,
  /* 250 FA  ú uacute          */ NULL,
  /* 251 FB  û ucircumflex     */ NULL,
  /* 252 FC  ü udieresis       */ NULL,
  /* 253 FD  ý yacute          */ NULL,
  /* 254 FE  þ thorn           */ NULL,
  /* 255 FF  ÿ ydieresis       */ NULL
};

static FILE *festival = NULL;

static void
spk_identify (void) {
  LogPrint(LOG_NOTICE, "Festival text to speech engine.");
}

static int
spk_open (char **parameters) {
  const char *command = parameters[PARM_COMMAND];
  if (!command || !*command) command = "festival";
  if ((festival = popen(command, "w"))) {
    fputs("(audio_mode 'async)\n", festival);
    fputs("(Parameter.set 'Audio_Method 'netaudio)\n", festival);

    {
      const char *name = parameters[PARM_NAME];
      if (name && *name) {
        if (strcasecmp(name, "Kevin") == 0) {
          fputs("(voice_ked_diphone)\n", festival);
        } else if (strcasecmp(name, "Kal") == 0) {
          fputs("(voice_kal_diphone)\n", festival);
        } else {
          LogPrint(LOG_WARNING, "Unknown Festival voice name: %s", name);
        }
      }
    }

    fflush(festival);
    return 1;
  }
  return 0;
}

static void
spk_close (void) {
  if (festival) {
    fputs("(quit)\n", festival);
    fflush(festival);
    pclose(festival);
    festival = NULL;
  }
}

static void
spk_say (const unsigned char *buffer, int length) {
  int index;

  fputs("(SayText \"", festival);
  for (index=0; index<length; index++) {
    unsigned char byte = buffer[index];
    const char *word = wordTable[byte];
    if (word) {
      fputs(word, festival);
    } else {
      fwrite(&byte, 1, 1, festival);
    }
  }
  fputs("\")\n", festival);
  fflush(festival);
}

static void
spk_mute (void) {
  fputs("(audio_mode 'shutup)\n", festival);
  fflush(festival);
}

static void
spk_rate (int setting) {
  char command[0X40];
  snprintf(command, sizeof(command), "(Parameter.set 'Duration_Stretch %f)\n",
           spkDurationStretchTable[setting]);
  fputs(command, festival);
  fflush(festival);
}
