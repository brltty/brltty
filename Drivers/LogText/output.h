/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* 00 */ 0X20, // mellemrum
/* 01 */ 0X61, // a at
/* 02 */ 0XB9, // an
/* 03 */ 0X63, // c og
/* 04 */ 0X2C, // , komma
/* 05 */ 0X62, // b bliver
/* 06 */ 0X69, // i
/* 07 */ 0X66, // f for
/* 08 */ 0XBF, // opl|sn.
/* 09 */ 0X65, // e eller
/* 0A */ 0XBA, // be
/* 0B */ 0X64, // d du
/* 0C */ 0X3A, // : kolon
/* 0D */ 0X68, // h har
/* 0E */ 0X6A, // j jeg
/* 0F */ 0X67, // g g|r
/* 10 */ 0X2E, // . punktum
/* 11 */ 0X6B, // k kan
/* 12 */ 0X98, // hvor
/* 13 */ 0X6D, // m med
/* 14 */ 0X3B, // ; semikol.
/* 15 */ 0X6C, // l lige
/* 16 */ 0X73, // s som
/* 17 */ 0X70, // p p}
/* 18 */ 0XCC, // igen
/* 19 */ 0X6F, // o op
/* 1A */ 0X91, // { v{re
/* 1B */ 0X6E, // n n}r
/* 1C */ 0XCA, // fra
/* 1D */ 0X72, // r rigtig
/* 1E */ 0X74, // t til
/* 1F */ 0X71, // q under
/* 20 */ 0X27, // ' apostrof
/* 21 */ 0X86, // } s}
/* 22 */ 0XBB, // ke
/* 23 */ 0X8C, // men
/* 24 */ 0X3F, // ? sp|rgsm.
/* 25 */ 0X88, // en
/* 26 */ 0X9B, // | f|r
/* 27 */ 0X89, // ned
/* 28 */ 0X21, // ! udr}b
/* 29 */ 0X96, // er
/* 2A */ 0XBC, // le
/* 2B */ 0X93, // de
/* 2C */ 0XC9, // deres
/* 2D */ 0X81, // te
/* 2E */ 0X77, // w hvad
/* 2F */ 0X8B, // gennem
/* 30 */ 0XCD, // var
/* 31 */ 0X75, // u hun
/* 32 */ 0XDB, // et
/* 33 */ 0X78, // x over
/* 34 */ 0XCB, // ham
/* 35 */ 0X76, // v ved
/* 36 */ 0X8A, // det
/* 37 */ 0X87, // den
/* 38 */ 0XC8, // af
/* 39 */ 0X7A, // z efter
/* 3A */ 0XDC, // ve
/* 3B */ 0X79, // y han
/* 3C */ 0X22, // " anf|rsel
/* 3D */ 0X85, // ret
/* 3E */ 0X97, // der
/* 3F */ 0X82, // skal
/* 40 */ 0X1A, // SUB (should be 0XFF but that would cause problems)
/* 41 */ 0X41, // A At
/* 42 */ 0XB1, // An
/* 43 */ 0X43, // C Og
/* 44 */ 0XA6, // 
/* 45 */ 0X42, // B Bliver
/* 46 */ 0X49, // I
/* 47 */ 0X46, // F For
/* 48 */ 0X8D, // 
/* 49 */ 0X45, // E Eller
/* 4A */ 0XB2, // Be
/* 4B */ 0X44, // D Du
/* 4C */ 0XB8, // 
/* 4D */ 0X48, // H Har
/* 4E */ 0X4A, // J Jeg
/* 4F */ 0X47, // G G|r
/* 50 */ 0X9F, // 
/* 51 */ 0X4B, // K Kan
/* 52 */ 0XB0, // Hvor
/* 53 */ 0X4D, // M Med
/* 54 */ 0XC6, // 
/* 55 */ 0X4C, // L Lige
/* 56 */ 0X53, // S Som
/* 57 */ 0X50, // P P}
/* 58 */ 0XC4, // Igen
/* 59 */ 0X4F, // O Op
/* 5A */ 0X92, // [ V{re
/* 5B */ 0X4E, // N N}r
/* 5C */ 0XC2, // Fra
/* 5D */ 0X52, // R Rigtig
/* 5E */ 0X54, // T Til
/* 5F */ 0X51, // Q Under
/* 60 */ 0X5E, // ^ hat
/* 61 */ 0X8F, // ] S}
/* 62 */ 0XB3, // Ke
/* 63 */ 0XD7, // Men
/* 64 */ 0XA7, // 
/* 65 */ 0XD2, // En
/* 66 */ 0X9D, // \ F|r
/* 67 */ 0XD3, // Ned
/* 68 */ 0X84, // 
/* 69 */ 0XEA, // Er
/* 6A */ 0XB4, // Le
/* 6B */ 0XE2, // De
/* 6C */ 0XC1, // Deres
/* 6D */ 0X9A, // Te
/* 6E */ 0X57, // W Hvad
/* 6F */ 0XD8, // Gennem
/* 70 */ 0XC5, // Var
/* 71 */ 0X55, // U Hun
/* 72 */ 0XD9, // Et
/* 73 */ 0X58, // X Over
/* 74 */ 0XC3, // Ham
/* 75 */ 0X56, // V Ved
/* 76 */ 0XD4, // Det
/* 77 */ 0X80, // Den
/* 78 */ 0XC0, // Af
/* 79 */ 0X5A, // Z Efter
/* 7A */ 0XDA, // Ve
/* 7B */ 0X59, // Y Han
/* 7C */ 0X60, // ` acc.grav
/* 7D */ 0XB7, // Ret
/* 7E */ 0XEB, // Der
/* 7F */ 0X90, // Skal
/* 80 */ 0X7F, // 
/* 81 */ 0X31, // 1 en,et
/* 82 */ 0XF8, // 
/* 83 */ 0X33, // 3 tre
/* 84 */ 0X83, // 
/* 85 */ 0X32, // 2 to
/* 86 */ 0X39, // 9 ni
/* 87 */ 0X36, // 6 seks
/* 88 */ 0XF9, // 
/* 89 */ 0X35, // 5 fem
/* 8A */ 0XFD, // 
/* 8B */ 0X34, // 4 fire
/* 8C */ 0X2F, // / h|j.skr}
/* 8D */ 0X38, // 8 otte
/* 8E */ 0X30, // 0 nul
/* 8F */ 0X37, // 7 syv
/* 90 */ 0XFA, // 
/* 91 */ 0XAB, // 
/* 92 */ 0XA1, // 
/* 93 */ 0X3C, // < mindre
/* 94 */ 0XAC, // 
/* 95 */ 0X9C, // 
/* 96 */ 0X3E, // > st|rre
/* 97 */ 0X9E, // 
/* 98 */ 0X94, // 
/* 99 */ 0X7D, // Ü tub slut
/* 9A */ 0X29, // ) h|j.par.
/* 9B */ 0XFC, // 
/* 9C */ 0X2B, // + plus
/* 9D */ 0XAF, // 
/* 9E */ 0XF4, // 
/* 9F */ 0XF1, // 
/* A0 */ 0X7E, // ~ tilde
/* A1 */ 0X5C, // ù backsl.
/* A2 */ 0XEF, // 
/* A3 */ 0XFB, // 
/* A4 */ 0XEE, // 
/* A5 */ 0X28, // ( ven.par.
/* A6 */ 0X7B, // ë tub beg
/* A7 */ 0X95, // 
/* A8 */ 0XE1, // 
/* A9 */ 0X24, // $ dollar
/* AA */ 0X7C, // õ lodret
/* AB */ 0X5D, // è h|j.kant
/* AC */ 0XF6, // 
/* AD */ 0XF5, // 
/* AE */ 0XAE, // 
/* AF */ 0XA4, // 
/* B0 */ 0X2D, // - min,bind
/* B1 */ 0XEC, // 
/* B2 */ 0XA2, // 
/* B3 */ 0X2A, // * stjerne
/* B4 */ 0XFE, // 
/* B5 */ 0X5B, // í ven.kant
/* B6 */ 0XF3, // 
/* B7 */ 0X26, // & amper
/* B8 */ 0X25, // % procent
/* B9 */ 0XF2, // 
/* BA */ 0X23, // # nummer
/* BB */ 0XF7, // 
/* BC */ 0X3D, // = lig med
/* BD */ 0XA0, // 
/* BE */ 0XA3, // 
/* BF */ 0XF0, // 
/* C0 */ 0X5F, // _ und.str.
/* C1 */ 0X01, // SOH
/* C2 */ 0X40, // @ master
/* C3 */ 0X03, // ETX
/* C4 */ 0XB6, // 
/* C5 */ 0X02, // STX
/* C6 */ 0X09, // HT
/* C7 */ 0X06, // ACK
/* C8 */ 0XDE, // 
/* C9 */ 0X05, // ENQ
/* CA */ 0X1E, // RS
/* CB */ 0X04, // EOT
/* CC */ 0XBD, // 
/* CD */ 0X08, // BS
/* CE */ 0X0A, // LF
/* CF */ 0X07, // BEL
/* D0 */ 0XA9, // 
/* D1 */ 0X0B, // VT
/* D2 */ 0XD6, // 
/* D3 */ 0X0D, // CR
/* D4 */ 0XC7, // 
/* D5 */ 0X0C, // FF
/* D6 */ 0X13, // DC3
/* D7 */ 0X10, // DLE
/* D8 */ 0X99, // 
/* D9 */ 0X0F, // SI
/* DA */ 0X1B, // ESC
/* DB */ 0X0E, // SO
/* DC */ 0XAD, // 
/* DD */ 0X12, // DC2
/* DE */ 0X14, // DC4
/* DF */ 0X11, // DC1
/* E0 */ 0XAA, // 
/* E1 */ 0X1D, // GS
/* E2 */ 0XCE, // 
/* E3 */ 0XE6, // 
/* E4 */ 0XA8, // 
/* E5 */ 0XCF, // 
/* E6 */ 0X1C, // FS
/* E7 */ 0XE3, // 
/* E8 */ 0X8E, // 
/* E9 */ 0XE4, // 
/* EA */ 0XD5, // 
/* EB */ 0XE8, // 
/* EC */ 0XED, // 
/* ED */ 0XE7, // 
/* EE */ 0X17, // ETB
/* EF */ 0XA5, // 
/* F0 */ 0X1F, // US
/* F1 */ 0X15, // NAC
/* F2 */ 0XE0, // 
/* F3 */ 0X18, // CAN
/* F4 */ 0XE5, // 
/* F5 */ 0X16, // SYN
/* F6 */ 0XD0, // 
/* F7 */ 0XBE, // 
/* F8 */ 0XD1, // 
/* F9 */ 0X1A, // SUB
/* FA */ 0XDD, // 
/* FB */ 0X19, // EM
/* FC */ 0XDF, // 
/* FD */ 0XB5, // 
/* FE */ 0XE9, // 
/* FF */ 0X00  // NUL
