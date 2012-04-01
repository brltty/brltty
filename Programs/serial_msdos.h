/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_SERIAL_MSDOS
#define BRLTTY_INCLUDED_SERIAL_MSDOS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  unsigned short divisor;
  unsigned short biosBPS;
} SerialSpeed;

typedef union {
  unsigned char byte;

  struct {
    unsigned bits:2;
    unsigned stop:1;
    unsigned parity:2;
    unsigned bps:3;
  } fields;
} SerialBiosConfiguration;

typedef struct {
  SerialBiosConfiguration bios;
  SerialSpeed speed;
} SerialAttributes;

typedef unsigned char SerialLines;
#define SERIAL_LINE_DTR 0X01
#define SERIAL_LINE_RTS 0X02
#define SERIAL_LINE_CTS 0X10
#define SERIAL_LINE_DSR 0X20
#define SERIAL_LINE_RNG 0X40
#define SERIAL_LINE_CAR 0X80

typedef struct {
  int deviceIndex;
} SerialPackageFields;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SERIAL_MSDOS */
