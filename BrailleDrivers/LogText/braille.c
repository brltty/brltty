/*
 * BRLTTY - A background process providing access to the console screen (when in
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

/* LogText/braille.c - Braille display library
 * For Tactilog's LogText
 * Author: Dave Mielke <dave@mielke.cc>
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "Programs/misc.h"

#define BRLSTAT ST_Generic
#include "Programs/brl_driver.h"
#include "Programs/tbl.h"
#include "braille.h"
#include "Programs/io_serial.h"

static SerialDevice *serialDevice = NULL;

#define screenHeight 25
#define screenWidth 80
typedef unsigned char ScreenImage[screenHeight][screenWidth];
static ScreenImage sourceImage;
static ScreenImage targetImage;

static const char *downloadPath = "logtext-download";

typedef enum {
   DEV_OFFLINE,
   DEV_ONLINE,
   DEV_READY
} DeviceStatus;
static DeviceStatus deviceStatus;

static BRL_DriverCommandContext currentContext;
static unsigned char currentLine;
static unsigned char cursorRow;
static unsigned char cursorColumn;

static TranslationTable outputTable = {
   [0] = 0X20, // mellemrum
   [BRL_DOT1] = 0X61, // a at
   [BRL_DOT1 | BRL_DOT2] = 0X62, // b bliver
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3] = 0X6C, // l lige
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4] = 0X70, // p p}
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X71, // q under
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X82, // skal
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X90, // Skal
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X00, // NUL
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XF0, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X51, // Q Under
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X11, // DC1
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XF1, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X87, // den
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X80, // Den
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XBE, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X26, // & amper
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X50, // P P}
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X10, // DLE
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X9E, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5] = 0X72, // r rigtig
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X85, // ret
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XB7, // Ret
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XB5, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XA0, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X52, // R Rigtig
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X12, // DC2
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0XAF, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6] = 0X76, // v ved
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X56, // V Ved
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X16, // SYN
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0X5B, //  ven.kant
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7] = 0X4C, // L Lige
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X0C, // FF
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT3 | BRL_DOT8] = 0X9C, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4] = 0X66, // f for
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5] = 0X67, // g g|r
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X8B, // gennem
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XD8, // Gennem
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XA5, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XA4, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X47, // G G|r
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X07, // BEL
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X37, // 7 syv
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6] = 0X89, // ned
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XD3, // Ned
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE3, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X95, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT7] = 0X46, // F For
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X06, // ACK
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT4 | BRL_DOT8] = 0X36, // 6 seks
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5] = 0X68, // h har
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6] = 0X81, // te
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X9A, // Te
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE7, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XF5, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT7] = 0X48, // H Har
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X08, // BS
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT5 | BRL_DOT8] = 0X38, // 8 otte
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT6] = 0X88, // en
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT7] = 0XD2, // En
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XCF, // 
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT6 | BRL_DOT8] = 0X28, // ( ven.par.
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT7] = 0X42, // B Bliver
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT7 | BRL_DOT8] = 0X02, // STX
   [BRL_DOT1 | BRL_DOT2 | BRL_DOT8] = 0X32, // 2 to
   [BRL_DOT1 | BRL_DOT3] = 0X6B, // k kan
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4] = 0X6D, // m med
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X6E, // n n}r
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X79, // y han
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X59, // Y Han
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X19, // EM
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XF7, // 
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X4E, // N N}r
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X0E, // SO
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XFC, // 
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X78, // x over
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X58, // X Over
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X18, // CAN
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X2A, // * stjerne
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X4D, // M Med
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X0D, // CR
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X3C, // < mindre
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5] = 0X6F, // o op
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X7A, // z efter
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X5A, // Z Efter
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X1A, // SUB
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XF2, // 
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0X4F, // O Op
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X0F, // SI
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X7D, //  tub slut
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT6] = 0X75, // u hun
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0X55, // U Hun
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X15, // NAC
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0XEC, // 
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT7] = 0X4B, // K Kan
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0X0B, // VT
   [BRL_DOT1 | BRL_DOT3 | BRL_DOT8] = 0XAB, // 
   [BRL_DOT1 | BRL_DOT4] = 0X63, // c og
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5] = 0X64, // d du
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X93, // de
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XE2, // De
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE8, // 
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X5D, //  h|j.kant
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X44, // D Du
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X04, // EOT
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X34, // 4 fire
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT6] = 0X8C, // men
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XD7, // Men
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE6, // 
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XFB, // 
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT7] = 0X43, // C Og
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X03, // ETX
   [BRL_DOT1 | BRL_DOT4 | BRL_DOT8] = 0X33, // 3 tre
   [BRL_DOT1 | BRL_DOT5] = 0X65, // e eller
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT6] = 0X96, // er
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XEA, // Er
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE4, // 
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X24, // $ dollar
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT7] = 0X45, // E Eller
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X05, // ENQ
   [BRL_DOT1 | BRL_DOT5 | BRL_DOT8] = 0X35, // 5 fem
   [BRL_DOT1 | BRL_DOT6] = 0X86, // } s}
   [BRL_DOT1 | BRL_DOT6 | BRL_DOT7] = 0X8F, // ] S}
   [BRL_DOT1 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X1D, // GS
   [BRL_DOT1 | BRL_DOT6 | BRL_DOT8] = 0X5C, //  backsl.
   [BRL_DOT1 | BRL_DOT7] = 0X41, // A At
   [BRL_DOT1 | BRL_DOT7 | BRL_DOT8] = 0X01, // SOH
   [BRL_DOT1 | BRL_DOT8] = 0X31, // 1 en,et
   [BRL_DOT2] = 0X2C, // , komma
   [BRL_DOT2 | BRL_DOT3] = 0X3B, // ; semikol.
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4] = 0X73, // s som
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X74, // t til
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X97, // der
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XEB, // Der
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE9, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XA3, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X54, // T Til
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X14, // DC4
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XF4, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0X8A, // det
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XD4, // Det
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XD0, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XF3, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0X53, // S Som
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X13, // DC3
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0X3E, // > st|rre
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5] = 0XCA, // fra
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0X22, // " anf|rsel
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X60, // ` acc.grav
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XDF, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X3D, // = lig med
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0XC2, // Fra
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0XAD, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X2B, // + plus
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT6] = 0XCB, // ham
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0XC3, // Ham
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE5, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0XFE, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT7] = 0XC6, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0XC7, // 
   [BRL_DOT2 | BRL_DOT3 | BRL_DOT8] = 0XAC, // 
   [BRL_DOT2 | BRL_DOT4] = 0X69, // i
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5] = 0X6A, // j jeg
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0X77, // w hvad
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X57, // W Hvad
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X17, // ETB
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XAE, // 
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X4A, // J Jeg
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X0A, // LF
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X30, // 0 nul
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT6] = 0X9B, // | f|r
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0X9D, // \ F|r
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X1C, // FS
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0X7B, //  tub beg
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT7] = 0X49, // I
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X09, // HT
   [BRL_DOT2 | BRL_DOT4 | BRL_DOT8] = 0X39, // 9 ni
   [BRL_DOT2 | BRL_DOT5] = 0X3A, // : kolon
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT6] = 0XC9, // deres
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XC1, // Deres
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XED, // 
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XF6, // 
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT7] = 0XB8, // 
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0XBD, // 
   [BRL_DOT2 | BRL_DOT5 | BRL_DOT8] = 0X2F, // / h|j.skr}
   [BRL_DOT2 | BRL_DOT6] = 0X3F, // ? sp|rgsm.
   [BRL_DOT2 | BRL_DOT6 | BRL_DOT7] = 0XA7, // 
   [BRL_DOT2 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XA8, // 
   [BRL_DOT2 | BRL_DOT6 | BRL_DOT8] = 0XEE, // 
   [BRL_DOT2 | BRL_DOT7] = 0XA6, // 
   [BRL_DOT2 | BRL_DOT7 | BRL_DOT8] = 0XB6, // 
   [BRL_DOT2 | BRL_DOT8] = 0X83, // 
   [BRL_DOT3] = 0X2E, // . punktum
   [BRL_DOT3 | BRL_DOT4] = 0X98, // hvor
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5] = 0X91, // { v{re
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0XDC, // ve
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XDA, // Ve
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XDD, // 
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X23, // # nummer
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0X92, // [ V{re
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X1B, // ESC
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0X29, // ) h|j.par.
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT6] = 0XDB, // et
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XD9, // Et
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XE0, // 
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XA2, // 
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT7] = 0XB0, // Hvor
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0XD6, // 
   [BRL_DOT3 | BRL_DOT4 | BRL_DOT8] = 0XA1, // 
   [BRL_DOT3 | BRL_DOT5] = 0XCC, // igen
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT6] = 0XC8, // af
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XC0, // Af
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XD1, // 
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X25, // % procent
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT7] = 0XC4, // Igen
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X99, // 
   [BRL_DOT3 | BRL_DOT5 | BRL_DOT8] = 0X94, // 
   [BRL_DOT3 | BRL_DOT6] = 0XCD, // var
   [BRL_DOT3 | BRL_DOT6 | BRL_DOT7] = 0XC5, // Var
   [BRL_DOT3 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X1F, // US
   [BRL_DOT3 | BRL_DOT6 | BRL_DOT8] = 0X2D, // - min,bind
   [BRL_DOT3 | BRL_DOT7] = 0X9F, // 
   [BRL_DOT3 | BRL_DOT7 | BRL_DOT8] = 0XA9, // 
   [BRL_DOT3 | BRL_DOT8] = 0XFA, // 
   [BRL_DOT4] = 0XB9, // an
   [BRL_DOT4 | BRL_DOT5] = 0XBA, // be
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT6] = 0XBC, // le
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0XB4, // Le
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XD5, // 
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0X7C, //  lodret
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT7] = 0XB2, // Be
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0X1E, // RS
   [BRL_DOT4 | BRL_DOT5 | BRL_DOT8] = 0XFD, // 
   [BRL_DOT4 | BRL_DOT6] = 0XBB, // ke
   [BRL_DOT4 | BRL_DOT6 | BRL_DOT7] = 0XB3, // Ke
   [BRL_DOT4 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XCE, // 
   [BRL_DOT4 | BRL_DOT6 | BRL_DOT8] = 0XEF, // 
   [BRL_DOT4 | BRL_DOT7] = 0XB1, // An
   [BRL_DOT4 | BRL_DOT7 | BRL_DOT8] = 0X40, // @ master
   [BRL_DOT4 | BRL_DOT8] = 0XF8, // 
   [BRL_DOT5] = 0XBF, // opl|sn.
   [BRL_DOT5 | BRL_DOT6] = 0X21, // ! udr}b
   [BRL_DOT5 | BRL_DOT6 | BRL_DOT7] = 0X84, // 
   [BRL_DOT5 | BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0X8E, // 
   [BRL_DOT5 | BRL_DOT6 | BRL_DOT8] = 0XE1, // 
   [BRL_DOT5 | BRL_DOT7] = 0X8D, // 
   [BRL_DOT5 | BRL_DOT7 | BRL_DOT8] = 0XDE, // 
   [BRL_DOT5 | BRL_DOT8] = 0XF9, // 
   [BRL_DOT6] = 0X27, // ' apostrof
   [BRL_DOT6 | BRL_DOT7] = 0X5E, // ^ hat
   [BRL_DOT6 | BRL_DOT7 | BRL_DOT8] = 0XAA, // 
   [BRL_DOT6 | BRL_DOT8] = 0X7E, // ~ tilde
   [BRL_DOT7] = 0XFF, // (causes problems so converted to SUB)
   [BRL_DOT7 | BRL_DOT8] = 0X5F, // _ und.str.
   [BRL_DOT8] = 0X7F  // 
};
static TranslationTable inputTable;

static void
brl_identify (void) {
   LogPrint(LOG_NOTICE, "LogText Driver");
   LogPrint(LOG_INFO, "   Copyright (C) 2001 by Dave Mielke <dave@mielke.cc>");
}

#ifndef __MINGW32__
static int
makeFifo (const char *path, mode_t mode) {
   struct stat status;
   if (lstat(path, &status) != -1) {
      if (S_ISFIFO(status.st_mode)) return 1;
      LogPrint(LOG_ERR, "Download object not a FIFO: %s", path);
   } else if (errno == ENOENT) {
      mode_t mask = umask(0);
      int result = mkfifo(path, mode);
      int error = errno;
      umask(mask);
      if (result != -1) return 1;
      errno = error;
      LogError("Download FIFO creation");
   }
   return 0;
}
#endif /* __MINGW32__ */

static int
makeDownloadFifo (void) {
#ifdef __MINGW32__
   return 0;
#else /* __MINGW32__ */
   return makeFifo(downloadPath, S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH);
#endif /* __MINGW32__ */
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *device) {
   if (memchr(outputTable, 0XFF, sizeof(outputTable))) {
      reverseTranslationTable(outputTable, inputTable);
      outputTable[inputTable[0XFF]] = 0X1A;
   }

   if (!isSerialDevice(&device)) {
      unsupportedDevice(device);
      return 0;
   }

   makeDownloadFifo();
   if ((serialDevice = serialOpenDevice(device))) {
      if (serialRestartDevice(serialDevice, 9600)) {
         brl->y = screenHeight;
         brl->x = screenWidth;
         brl->buffer = &sourceImage[0][0];
         memset(sourceImage, 0, sizeof(sourceImage));
         deviceStatus = DEV_ONLINE;
         return 1;
      }
      serialCloseDevice(serialDevice);
      serialDevice = NULL;
   }
   return 0;
}

static void
brl_close (BrailleDisplay *brl) {
   serialCloseDevice(serialDevice);
   serialDevice = NULL;
}

static int
checkData (const unsigned char *data, unsigned int length) {
   if ((length < 5) || (length != (data[4] + 5))) {
      LogPrint(LOG_ERR, "Bad length: %d", length);
   } else if (data[0] != 255) {
      LogPrint(LOG_ERR, "Bad header: %d", data[0]);
   } else if ((data[1] < 1) || (data[1] > screenHeight)) {
      LogPrint(LOG_ERR, "Bad line: %d", data[1]);
   } else if (data[2] > screenWidth) {
      LogPrint(LOG_ERR, "Bad cursor: %d", data[2]);
   } else if ((data[3] < 1) || (data[3] > screenWidth)) {
      LogPrint(LOG_ERR, "Bad column: %d", data[3]);
   } else if (data[4] > (screenWidth - (data[3] - 1))) {
      LogPrint(LOG_ERR, "Bad count: %d", data[4]);
   } else {
      return 1;
   }
   return 0;
}

static int
sendBytes (const unsigned char *bytes, size_t count) {
   if (serialWriteData(serialDevice, bytes, count) == -1) {
      LogError("LogText write");
      return 0;
   }
   return 1;
}

static int
sendData (unsigned char line, unsigned char column, unsigned char count) {
   unsigned char data[5 + count];
   unsigned char *target = data;
   unsigned char *source = &targetImage[line][column];
   *target++ = 0XFF;
   *target++ = line + 1;
   *target++ = (line == cursorRow)? cursorColumn+1: 0;
   *target++ = column + 1;
   *target++ = count;
   LogBytes("Output dots", source, count);
   while (count--) *target++ = outputTable[*source++];
   count = target - data;
   LogBytes("LogText write", data, count);
   if (checkData(data, count)) {
      if (sendBytes(data, count)) {
         return 1;
      }
   }
   return 0;
}

static int
sendLine (unsigned char line, int force) {
   unsigned char *source = &sourceImage[line][0];
   unsigned char *target = &targetImage[line][0];
   unsigned char start = 0;
   unsigned char count = screenWidth;
   while (count > 0) {
      if (source[count-1] != target[count-1]) break;
      --count;
   }
   while (start < count) {
      if (source[start] != target[start]) break;
      ++start;
   }
   if ((count -= start) || force) {
      LogPrint(LOG_DEBUG, "LogText line: line=%d, column=%d, count=%d", line, start, count);
      memcpy(&target[start], &source[start], count);
      if (!sendData(line, start, count)) {
         return 0;
      }
   }
   return 1;
}

static int
sendCurrentLine (void) {
   return sendLine(currentLine, 0);
}

static int
sendCursorRow (void) {
   return sendLine(cursorRow, 1);
}

static int
handleUpdate (unsigned char line) {
   LogPrint(LOG_DEBUG, "Request line: (0X%2.2X) 0X%2.2X dec=%d", KEY_UPDATE, line, line);
   if (!line) return sendCursorRow();
   if (line <= screenHeight) {
      currentLine = line - 1;
      return sendCurrentLine();
   }
   LogPrint(LOG_WARNING, "Invalid line request: %d", line);
   return 1;
}

static void
brl_writeWindow (BrailleDisplay *brl) {
   if (deviceStatus == DEV_READY) {
      sendCurrentLine();
   }
}

static int
isOnline (void) {
   int online = serialTestLineDSR(serialDevice);
   if (online) {
      if (deviceStatus < DEV_ONLINE) {
         deviceStatus = DEV_ONLINE;
         LogPrint(LOG_WARNING, "LogText online.");
      }
   } else {
      if (deviceStatus > DEV_OFFLINE) {
         deviceStatus = DEV_OFFLINE;
         LogPrint(LOG_WARNING, "LogText offline.");
      }
   }
   return online;
}

static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
   if (isOnline()) {
      if (status[BRL_firstStatusCell] == BRL_STATUS_CELLS_GENERIC) {
         unsigned char row = status[BRL_GSC_CSRROW];
         unsigned char column = status[BRL_GSC_CSRCOL];
         row = MAX(1, MIN(row, screenHeight)) - 1;
         column = MAX(1, MIN(column, screenWidth)) - 1;
         if (deviceStatus < DEV_READY) {
            memset(targetImage, 0, sizeof(targetImage));
            currentContext = BRL_CTX_SCREEN;
            currentLine = row;
            cursorRow = screenHeight;
            cursorColumn = screenWidth;
            deviceStatus = DEV_READY;
         }
         if ((row != cursorRow) || (column != cursorColumn)) {
            LogPrint(LOG_DEBUG, "cursor moved: [%d,%d] -> [%d,%d]", cursorColumn, cursorRow, column, row);
            cursorRow = row;
            cursorColumn = column;
            sendCursorRow();
         }
      }
   }
}

static int
readKey (void) {
   unsigned char key;
   unsigned char arg;
   if (serialReadData(serialDevice, &key, 1, 0, 0) != 1) return EOF;
   switch (key) {
      default:
         arg = 0;
         break;
      case KEY_FUNCTION:
      case KEY_FUNCTION2:
      case KEY_UPDATE:
         while (serialReadData(serialDevice, &arg, 1, 0, 0) != 1) approximateDelay(1);
         break;
   }
   {
      int result = COMPOUND_KEY(key, arg);
      LogPrint(LOG_DEBUG, "Key read: %4.4X", result);
      return result;
   }
}

/*askUser
static unsigned char *selectedLine;

static void
replaceCharacters (const unsigned char *address, size_t count) {
   while (count--) selectedLine[cursorColumn++] = inputTable[*address++];
}

static void
insertCharacters (const unsigned char *address, size_t count) {
   memmove(&selectedLine[cursorColumn+count], &selectedLine[cursorColumn], screenWidth-cursorColumn-count);
   replaceCharacters(address, count);
}

static void
deleteCharacters (size_t count) {
   memmove(&selectedLine[cursorColumn], &selectedLine[cursorColumn+count], screenWidth-cursorColumn-count);
   memset(&selectedLine[screenWidth-count], inputTable[' '], count);
}

static void
clearCharacters (void) {
   cursorColumn = 0;
   deleteCharacters(screenWidth);
}

static void
selectLine (unsigned char line) {
   selectedLine = &sourceImage[cursorRow = line][0];
   clearCharacters();
   deviceStatus = DEV_ONLINE;
}

static unsigned char *
askUser (const unsigned char *prompt) {
   unsigned char from;
   unsigned char to;
   selectLine(screenHeight-1);
   LogPrint(LOG_DEBUG, "Prompt: %s", prompt);
   replaceCharacters(prompt, strlen(prompt));
   from = to = ++cursorColumn;
   sendCursorRow();
   while (1) {
      int key = readKey();
      if (key == EOF) {
         approximateDelay(1);
         continue;
      }
      if ((key & KEY_MASK) == KEY_UPDATE) {
         handleUpdate(key >> KEY_SHIFT);
         continue;
      }
      if (isgraph(key)) {
         if (to < screenWidth) {
            unsigned char character = key & KEY_MASK;
            insertCharacters(&character, 1);
            ++to;
         } else {
            ringBell();
         }
      } else {
         switch (key) {
            case 0X0D:
               if (to > from) {
                  size_t length = to - from;
                  unsigned char *response = malloc(length+1);
                  if (response) {
                     response[length] = 0;
                     do {
                        response[--length] = outputTable[selectedLine[--to]];
                     } while (to > from);
                     LogPrint(LOG_DEBUG, "Response: %s", response);
                     return response;
                  } else {
                     LogError("Download file path allocation");
                  }
               }
               return NULL;
            case 0X08:
               if (cursorColumn > from) {
                  --cursorColumn;
                  deleteCharacters(1);
                  --to;
               } else {
                  ringBell();
               }
               break;
            case 0X7F:
               if (cursorColumn < to) {
                  deleteCharacters(1);
                  --to;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_LEFT:
               if (cursorColumn > from) {
                  --cursorColumn;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_LEFT_JUMP:
               if (cursorColumn > from) {
                  cursorColumn = from;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_RIGHT:
               if (cursorColumn < to) {
                  ++cursorColumn;
               } else {
                  ringBell();
               }
               break;
            case KEY_FUNCTION_CURSOR_RIGHT_JUMP:
               if (cursorColumn < to) {
                  cursorColumn = to;
               } else {
                  ringBell();
               }
               break;
            default:
               ringBell();
               break;
         }
      }
      sendCursorRow();
   }
}
*/

static void
downloadFile (void) {
   if (makeDownloadFifo()) {
      int file = open(downloadPath, O_RDONLY);
      if (file != -1) {
         struct stat status;
         if (fstat(file, &status) != -1) {
            unsigned char buffer[0X400];
            const unsigned char *address = buffer;
            int count = 0;
            while (1) {
               const unsigned char *newline;
               if (!count) {
                  count = read(file, buffer, sizeof(buffer));
                  if (!count) {
                     static const unsigned char fileTrailer[] = {0X1A};
                     sendBytes(fileTrailer, sizeof(fileTrailer));
                     break;
                  }
                  if (count == -1) {
                     LogError("Download file read");
                     break;
                  }
                  address = buffer;
               }
               if ((newline = memchr(address, '\n', count))) {
                  static const unsigned char lineTrailer[] = {0X0D, 0X0A};
                  size_t length = newline - address;
                  if (!sendBytes(address, length++)) break;
                  if (!sendBytes(lineTrailer, sizeof(lineTrailer))) break;
                  address += length;
                  count -= length;
               } else {
                  if (!sendBytes(address, count)) break;
                  count = 0;
               }
            }
         } else {
            LogError("Download file status");
         }
         if (close(file) == -1) {
            LogError("Download file close");
         }
      } else {
         LogError("Download file open");
      }
   } else {
      LogPrint(LOG_WARNING, "Download path not specified.");
   }
}

static int
brl_readCommand (BrailleDisplay *brl, BRL_DriverCommandContext context) {
   int key = readKey();
   if (context != currentContext) {
      LogPrint(LOG_DEBUG, "Context switch: %d -> %d", currentContext, context);
      switch (currentContext = context) {
         case BRL_CTX_SCREEN:
            deviceStatus = DEV_ONLINE;
            break;
         default:
            break;
      }
   }
   if (key != EOF) {
      switch (key) {
         case KEY_FUNCTION_ENTER:
            return BRL_BLK_PASSKEY + BRL_KEY_ENTER;
         case KEY_FUNCTION_TAB:
            return BRL_BLK_PASSKEY + BRL_KEY_TAB;
         case KEY_FUNCTION_CURSOR_UP:
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP;
         case KEY_FUNCTION_CURSOR_DOWN:
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN;
         case KEY_FUNCTION_CURSOR_LEFT:
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT;
         case KEY_FUNCTION_CURSOR_RIGHT:
            return BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT;
         case KEY_FUNCTION_CURSOR_UP_JUMP:
            return BRL_BLK_PASSKEY + BRL_KEY_HOME;
         case KEY_FUNCTION_CURSOR_DOWN_JUMP:
            return BRL_BLK_PASSKEY + BRL_KEY_END;
         case KEY_FUNCTION_CURSOR_LEFT_JUMP:
            return BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
         case KEY_FUNCTION_CURSOR_RIGHT_JUMP:
            return BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
         case KEY_FUNCTION_F1:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 0;
         case KEY_FUNCTION_F2:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 1;
         case KEY_FUNCTION_F3:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 2;
         case KEY_FUNCTION_F4:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 3;
         case KEY_FUNCTION_F5:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 4;
         case KEY_FUNCTION_F6:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 5;
         case KEY_FUNCTION_F7:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 6;
         case KEY_FUNCTION_F9:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 8;
         case KEY_FUNCTION_F10:
            return BRL_BLK_PASSKEY + BRL_KEY_FUNCTION + 9;
         case KEY_COMMAND: {
            int command;
            while ((command = readKey()) == EOF) approximateDelay(1);
            LogPrint(LOG_DEBUG, "Received command: (0x%2.2X) 0x%4.4X", KEY_COMMAND, command);
            switch (command) {
               case KEY_COMMAND:
                  /* pressing the escape command twice will pass it through */
                  return BRL_BLK_PASSDOTS + inputTable[KEY_COMMAND];
               case KEY_COMMAND_SWITCHVT_PREV:
                  return BRL_CMD_SWITCHVT_PREV;
               case KEY_COMMAND_SWITCHVT_NEXT:
                  return BRL_CMD_SWITCHVT_NEXT;
               case KEY_COMMAND_SWITCHVT_1:
                  return BRL_BLK_SWITCHVT + 0;
               case KEY_COMMAND_SWITCHVT_2:
                  return BRL_BLK_SWITCHVT + 1;
               case KEY_COMMAND_SWITCHVT_3:
                  return BRL_BLK_SWITCHVT + 2;
               case KEY_COMMAND_SWITCHVT_4:
                  return BRL_BLK_SWITCHVT + 3;
               case KEY_COMMAND_SWITCHVT_5:
                  return BRL_BLK_SWITCHVT + 4;
               case KEY_COMMAND_SWITCHVT_6:
                  return BRL_BLK_SWITCHVT + 5;
               case KEY_COMMAND_SWITCHVT_7:
                  return BRL_BLK_SWITCHVT + 6;
               case KEY_COMMAND_SWITCHVT_8:
                  return BRL_BLK_SWITCHVT + 7;
               case KEY_COMMAND_SWITCHVT_9:
                  return BRL_BLK_SWITCHVT + 8;
               case KEY_COMMAND_SWITCHVT_10:
                  return BRL_BLK_SWITCHVT + 9;
               case KEY_COMMAND_PAGE_UP:
                  return BRL_BLK_PASSKEY + BRL_KEY_PAGE_UP;
               case KEY_COMMAND_PAGE_DOWN:
                  return BRL_BLK_PASSKEY + BRL_KEY_PAGE_DOWN;
               case KEY_COMMAND_PREFMENU:
                  currentLine = 0;
                  cursorRow = 0;
                  cursorColumn = 31;
                  sendCursorRow();
                  return BRL_CMD_PREFMENU;
               case KEY_COMMAND_PREFSAVE:
                  return BRL_CMD_PREFSAVE;
               case KEY_COMMAND_PREFLOAD:
                  return BRL_CMD_PREFLOAD;
               case KEY_COMMAND_FREEZE_ON:
                  return BRL_CMD_FREEZE | BRL_FLG_TOGGLE_ON;
               case KEY_COMMAND_FREEZE_OFF:
                  return BRL_CMD_FREEZE | BRL_FLG_TOGGLE_OFF;
               case KEY_COMMAND_RESTARTBRL:
                  return BRL_CMD_RESTARTBRL;
               case KEY_COMMAND_DOWNLOAD:
                  downloadFile();
                  break;
               default:
                  LogPrint(LOG_WARNING, "Unknown command: (0X%2.2X) 0X%4.4X", KEY_COMMAND, command);
                  break;
            }
            break;
         }
         default:
            switch (key & KEY_MASK) {
               case KEY_UPDATE:
                  handleUpdate(key >> KEY_SHIFT);
                  break;
               case KEY_FUNCTION:
                  LogPrint(LOG_WARNING, "Unknown function: (0X%2.2X) 0X%4.4X", KEY_COMMAND, key>>KEY_SHIFT);
                  break;
               default: {
                  unsigned char dots = inputTable[key];
                  LogPrint(LOG_DEBUG, "Received character: 0X%2.2X dec=%d dots=%2.2X", key, key, dots);
                  return BRL_BLK_PASSDOTS + dots;
               }
            }
            break;
      }
   }
   return EOF;
}
