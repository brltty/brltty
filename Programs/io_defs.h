/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2006 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_IO_DEFS
#define BRLTTY_INCLUDED_IO_DEFS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  SERIAL_PARITY_SPACE,
  SERIAL_PARITY_ODD,
  SERIAL_PARITY_EVEN,
  SERIAL_PARITY_MARK,
  SERIAL_PARITY_NONE
} SerialParity;

typedef enum {
  SERIAL_FLOW_OUTPUT_XON = 0X001, /* output controlled by X-ON/X-OFF(input) */
  SERIAL_FLOW_OUTPUT_CTS = 0X002, /* output controlled by CTS(input) */
  SERIAL_FLOW_OUTPUT_DSR = 0X004, /* output controlled by DSR(input) */
  SERIAL_FLOW_OUTPUT_RTS = 0X008, /* output indicated by RTS(output) */

  SERIAL_FLOW_INPUT_XON  = 0X010, /* input controlled by X-ON/X-OFF(output) */
  SERIAL_FLOW_INPUT_RTS  = 0X020, /* input controlled by RTS(output) */
  SERIAL_FLOW_INPUT_DTR  = 0X040, /* input controlled by DTR(output) */
  SERIAL_FLOW_INPUT_DSR  = 0X080, /* input enabled by DSR(input) */

  SERIAL_FLOW_INPUT_CTS  = 0X100, /* input indicated by CTS(input) */

  SERIAL_FLOW_HARDWARE   = (SERIAL_FLOW_OUTPUT_CTS | SERIAL_FLOW_INPUT_RTS),

  SERIAL_FLOW_NONE       = 0X00  /* no input or output flow control */
} SerialFlowControl;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_IO_DEFS */
