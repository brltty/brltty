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

/* 00 */ 0XFF, // NUL
/* 01 */ 0XC1, // SOH
/* 02 */ 0XC5, // STX
/* 03 */ 0XC3, // ETX
/* 04 */ 0XCB, // EOT
/* 05 */ 0XC9, // ENQ
/* 06 */ 0XC7, // ACK
/* 07 */ 0XCF, // BEL
/* 08 */ 0XCD, // BS
/* 09 */ 0XC6, // HT
/* 0A */ 0XCE, // LF
/* 0B */ 0XD1, // VT
/* 0C */ 0XD5, // FF
/* 0D */ 0XD3, // CR
/* 0E */ 0XDB, // SO
/* 0F */ 0XD9, // SI
/* 10 */ 0XD7, // DLE
/* 11 */ 0XDF, // DC1
/* 12 */ 0XDD, // DC2
/* 13 */ 0XD6, // DC3
/* 14 */ 0XDE, // DC4
/* 15 */ 0XF1, // NAC
/* 16 */ 0XF5, // SYN
/* 17 */ 0XEE, // ETB
/* 18 */ 0XF3, // CAN
/* 19 */ 0XFB, // EM
/* 1A */ 0XF9, // SUB
/* 1B */ 0XDA, // ESC
/* 1C */ 0XE6, // FS
/* 1D */ 0XE1, // GS
/* 1E */ 0XCA, // RS
/* 1F */ 0XF0, // US
/* 20 */ 0X00, // mellemrum
/* 21 */ 0X28, // ! udr}b
/* 22 */ 0X3C, // " anf|rsel
/* 23 */ 0XBA, // # nummer
/* 24 */ 0XA9, // $ dollar
/* 25 */ 0XB8, // % procent
/* 26 */ 0XB7, // & amper
/* 27 */ 0X20, // ' apostrof
/* 28 */ 0XA5, // ( ven.par.
/* 29 */ 0X9A, // ) h|j.par.
/* 2A */ 0XB3, // * stjerne
/* 2B */ 0X9C, // + plus
/* 2C */ 0X04, // , komma
/* 2D */ 0XB0, // - min,bind
/* 2E */ 0X10, // . punktum
/* 2F */ 0X8C, // / h|j.skr}
/* 30 */ 0X8E, // 0 nul
/* 31 */ 0X81, // 1 en,et
/* 32 */ 0X85, // 2 to
/* 33 */ 0X83, // 3 tre
/* 34 */ 0X8B, // 4 fire
/* 35 */ 0X89, // 5 fem
/* 36 */ 0X87, // 6 seks
/* 37 */ 0X8F, // 7 syv
/* 38 */ 0X8D, // 8 otte
/* 39 */ 0X86, // 9 ni
/* 3A */ 0X0C, // : kolon
/* 3B */ 0X14, // ; semikol.
/* 3C */ 0X93, // < mindre
/* 3D */ 0XBC, // = lig med
/* 3E */ 0X96, // > st|rre
/* 3F */ 0X24, // ? sp|rgsm.
/* 40 */ 0XC2, // @ master
/* 41 */ 0X41, // A At
/* 42 */ 0X45, // B Bliver
/* 43 */ 0X43, // C Og
/* 44 */ 0X4B, // D Du
/* 45 */ 0X49, // E Eller
/* 46 */ 0X47, // F For
/* 47 */ 0X4F, // G G|r
/* 48 */ 0X4D, // H Har
/* 49 */ 0X46, // I
/* 4A */ 0X4E, // J Jeg
/* 4B */ 0X51, // K Kan
/* 4C */ 0X55, // L Lige
/* 4D */ 0X53, // M Med
/* 4E */ 0X5B, // N N}r
/* 4F */ 0X59, // O Op
/* 50 */ 0X57, // P P}
/* 51 */ 0X5F, // Q Under
/* 52 */ 0X5D, // R Rigtig
/* 53 */ 0X56, // S Som
/* 54 */ 0X5E, // T Til
/* 55 */ 0X71, // U Hun
/* 56 */ 0X75, // V Ved
/* 57 */ 0X6E, // W Hvad
/* 58 */ 0X73, // X Over
/* 59 */ 0X7B, // Y Han
/* 5A */ 0X79, // Z Efter
/* 5B */ 0XB5, // í ven.kant
/* 5C */ 0XA1, // ù backsl.
/* 5D */ 0XAB, // è h|j.kant
/* 5E */ 0X60, // ^ hat
/* 5F */ 0XC0, // _ und.str.
/* 60 */ 0X7C, // ` acc.grav
/* 61 */ 0X01, // a at
/* 62 */ 0X05, // b bliver
/* 63 */ 0X03, // c og
/* 64 */ 0X0B, // d du
/* 65 */ 0X09, // e eller
/* 66 */ 0X07, // f for
/* 67 */ 0X0F, // g g|r
/* 68 */ 0X0D, // h har
/* 69 */ 0X06, // i
/* 6A */ 0X0E, // j jeg
/* 6B */ 0X11, // k kan
/* 6C */ 0X15, // l lige
/* 6D */ 0X13, // m med
/* 6E */ 0X1B, // n n}r
/* 6F */ 0X19, // o op
/* 70 */ 0X17, // p p}
/* 71 */ 0X1F, // q under
/* 72 */ 0X1D, // r rigtig
/* 73 */ 0X16, // s som
/* 74 */ 0X1E, // t til
/* 75 */ 0X31, // u hun
/* 76 */ 0X35, // v ved
/* 77 */ 0X2E, // w hvad
/* 78 */ 0X33, // x over
/* 79 */ 0X3B, // y han
/* 7A */ 0X39, // z efter
/* 7B */ 0XA6, // ë tub beg
/* 7C */ 0XAA, // õ lodret
/* 7D */ 0X99, // Ü tub slut
/* 7E */ 0XA0, // ~ tilde
/* 7F */ 0X80, // 
/* 80 */ 0X77, // Den
/* 81 */ 0X2D, // te
/* 82 */ 0X3F, // skal
/* 83 */ 0X84, // 
/* 84 */ 0X68, // 
/* 85 */ 0X3D, // ret
/* 86 */ 0X21, // } s}
/* 87 */ 0X37, // den
/* 88 */ 0X25, // en
/* 89 */ 0X27, // ned
/* 8A */ 0X36, // det
/* 8B */ 0X2F, // gennem
/* 8C */ 0X23, // men
/* 8D */ 0X48, // 
/* 8E */ 0XE8, // 
/* 8F */ 0X61, // ] S}
/* 90 */ 0X7F, // Skal
/* 91 */ 0X1A, // { v{re
/* 92 */ 0X5A, // [ V{re
/* 93 */ 0X2B, // de
/* 94 */ 0X98, // 
/* 95 */ 0XA7, // 
/* 96 */ 0X29, // er
/* 97 */ 0X3E, // der
/* 98 */ 0X12, // hvor
/* 99 */ 0XD8, // 
/* 9A */ 0X6D, // Te
/* 9B */ 0X26, // | f|r
/* 9C */ 0X95, // 
/* 9D */ 0X66, // \ F|r
/* 9E */ 0X97, // 
/* 9F */ 0X50, // 
/* A0 */ 0XBD, // 
/* A1 */ 0X92, // 
/* A2 */ 0XB2, // 
/* A3 */ 0XBE, // 
/* A4 */ 0XAF, // 
/* A5 */ 0XEF, // 
/* A6 */ 0X44, // 
/* A7 */ 0X64, // 
/* A8 */ 0XE4, // 
/* A9 */ 0XD0, // 
/* AA */ 0XE0, // 
/* AB */ 0X91, // 
/* AC */ 0X94, // 
/* AD */ 0XDC, // 
/* AE */ 0XAE, // 
/* AF */ 0X9D, // 
/* B0 */ 0X52, // Hvor
/* B1 */ 0X42, // An
/* B2 */ 0X4A, // Be
/* B3 */ 0X62, // Ke
/* B4 */ 0X6A, // Le
/* B5 */ 0XFD, // 
/* B6 */ 0XC4, // 
/* B7 */ 0X7D, // Ret
/* B8 */ 0X4C, // 
/* B9 */ 0X02, // an
/* BA */ 0X0A, // be
/* BB */ 0X22, // ke
/* BC */ 0X2A, // le
/* BD */ 0XCC, // 
/* BE */ 0XF7, // 
/* BF */ 0X08, // opl|sn.
/* C0 */ 0X78, // Af
/* C1 */ 0X6C, // Deres
/* C2 */ 0X5C, // Fra
/* C3 */ 0X74, // Ham
/* C4 */ 0X58, // Igen
/* C5 */ 0X70, // Var
/* C6 */ 0X54, // 
/* C7 */ 0XD4, // 
/* C8 */ 0X38, // af
/* C9 */ 0X2C, // deres
/* CA */ 0X1C, // fra
/* CB */ 0X34, // ham
/* CC */ 0X18, // igen
/* CD */ 0X30, // var
/* CE */ 0XE2, // 
/* CF */ 0XE5, // 
/* D0 */ 0XF6, // 
/* D1 */ 0XF8, // 
/* D2 */ 0X65, // En
/* D3 */ 0X67, // Ned
/* D4 */ 0X76, // Det
/* D5 */ 0XEA, // 
/* D6 */ 0XD2, // 
/* D7 */ 0X63, // Men
/* D8 */ 0X6F, // Gennem
/* D9 */ 0X72, // Et
/* DA */ 0X7A, // Ve
/* DB */ 0X32, // et
/* DC */ 0X3A, // ve
/* DD */ 0XFA, // 
/* DE */ 0XC8, // 
/* DF */ 0XFC, // 
/* E0 */ 0XF2, // 
/* E1 */ 0XA8, // 
/* E2 */ 0X6B, // De
/* E3 */ 0XE7, // 
/* E4 */ 0XE9, // 
/* E5 */ 0XF4, // 
/* E6 */ 0XE3, // 
/* E7 */ 0XED, // 
/* E8 */ 0XEB, // 
/* E9 */ 0XFE, // 
/* EA */ 0X69, // Er
/* EB */ 0X7E, // Der
/* EC */ 0XB1, // 
/* ED */ 0XEC, // 
/* EE */ 0XA4, // 
/* EF */ 0XA2, // 
/* F0 */ 0XBF, // 
/* F1 */ 0X9F, // 
/* F2 */ 0XB9, // 
/* F3 */ 0XB6, // 
/* F4 */ 0X9E, // 
/* F5 */ 0XAD, // 
/* F6 */ 0XAC, // 
/* F7 */ 0XBB, // 
/* F8 */ 0X82, // 
/* F9 */ 0X88, // 
/* FA */ 0X90, // 
/* FB */ 0XA3, // 
/* FC */ 0X9B, // 
/* FD */ 0X8A, // 
/* FE */ 0XB4, // 
/* FF */ 0X40  // 
