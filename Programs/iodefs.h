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

#ifndef _IODEFS_H
#define _IODEFS_H

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
  SERIAL_FLOW_OUTPUT_XON = 0X01, /* output controlled by XON/XOFF */
  SERIAL_FLOW_OUTPUT_CTS = 0X02, /* output controlled by CTS(input) */
  SERIAL_FLOW_OUTPUT_DSR = 0X04, /* output controlled by DSR(input) */
  SERIAL_FLOW_OUTPUT_RTS = 0X08, /* output indicated by RTS(output) */
  SERIAL_FLOW_INPUT_XON  = 0X10, /* input controlled by XON/XOFF */
  SERIAL_FLOW_INPUT_RTS  = 0X20, /* input controlled by RTS(output) */
  SERIAL_FLOW_INPUT_DTR  = 0X40, /* input controlled by DTR(output) */
  SERIAL_FLOW_INPUT_DSR  = 0X80, /* input enabled by DSR(input) */
  SERIAL_FLOW_NONE       = 0X00  /* no input or output flow control */
} SerialFlowControl;
#define SERIAL_FLOW_HARDWARE (SERIAL_FLOW_OUTPUT_CTS | SERIAL_FLOW_INPUT_RTS)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _IODEFS_H */
