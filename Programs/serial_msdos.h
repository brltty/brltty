/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2013 by The BRLTTY Developers.
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

#include "serial_uart.h"

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
#define SERIAL_LINE_DTR UART_FLAG_MCR_DTR
#define SERIAL_LINE_RTS UART_FLAG_MCR_RTS
#define SERIAL_LINE_CTS UART_FLAG_MSR_CTS
#define SERIAL_LINE_DSR UART_FLAG_MSR_DSR
#define SERIAL_LINE_RNG UART_FLAG_MSR_RNG
#define SERIAL_LINE_CAR UART_FLAG_MSR_CAR

typedef struct {
  int deviceIndex;
} SerialPackageFields;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_SERIAL_MSDOS */
