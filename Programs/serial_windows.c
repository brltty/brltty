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

#include "prologue.h"

#include "serial_internal.h"
#include "ascii.h"

BEGIN_SERIAL_BAUD_TABLE
#ifdef CBR_110
  {110, CBR_110},
#endif /* CBR_110 */

#ifdef CBR_300
  {300, CBR_300},
#endif /* CBR_300 */

#ifdef CBR_600
  {600, CBR_600},
#endif /* CBR_600 */

#ifdef CBR_1200
  {1200, CBR_1200},
#endif /* CBR_1200 */

#ifdef CBR_2400
  {2400, CBR_2400},
#endif /* CBR_2400 */

#ifdef CBR_4800
  {4800, CBR_4800},
#endif /* CBR_4800 */

#ifdef CBR_9600
  {9600, CBR_9600},
#endif /* CBR_9600 */

#ifdef CBR_14400
  {14400, CBR_14400},
#endif /* CBR_14400 */

#ifdef CBR_19200
  {19200, CBR_19200},
#endif /* CBR_19200 */

#ifdef CBR_38400
  {38400, CBR_38400},
#endif /* CBR_38400 */

#ifdef CBR_56000
  {56000, CBR_56000},
#endif /* CBR_56000 */

#ifdef CBR_57600
  {57600, CBR_57600},
#endif /* CBR_57600 */

#ifdef CBR_115200
  {115200, CBR_115200},
#endif /* CBR_115200 */

#ifdef CBR_128000
  {128000, CBR_128000},
#endif /* CBR_128000 */

#ifdef CBR_256000
  {256000, CBR_256000},
#endif /* CBR_256000 */
END_SERIAL_BAUD_TABLE

void
serialPutInitialAttributes (SerialAttributes *attributes) {
  attributes->DCBlength = sizeof(*attributes);
  attributes->fBinary = TRUE;
  attributes->fTXContinueOnXoff = TRUE;
  attributes->XonChar = DC1;
  attributes->XoffChar = DC3;
}

int
serialPutSpeed (SerialAttributes *attributes, SerialSpeed speed) {
  attributes->BaudRate = speed;
  return 1;
}

int
serialPutDataBits (SerialAttributes *attributes, unsigned int bits) {
  if ((bits < 5) || (bits > 8)) return 0;
  attributes->ByteSize = bits;
  return 1;
}

int
serialPutStopBits (SerialAttributes *attributes, SerialStopBits bits) {
  if (bits == SERIAL_STOP_1) {
    attributes->StopBits = ONESTOPBIT;
  } else if (bits == SERIAL_STOP_1_5) {
    attributes->StopBits = ONE5STOPBITS;
  } else if (bits == SERIAL_STOP_2) {
    attributes->StopBits = TWOSTOPBITS;
  } else {
    return 0;
  }

  return 1;
}

int
serialPutParity (SerialAttributes *attributes, SerialParity parity) {
  attributes->fParity = FALSE;
  attributes->Parity = NOPARITY;

  if (parity != SERIAL_PARITY_NONE) {
    switch (parity) {
      case SERIAL_PARITY_ODD:
        attributes->Parity = ODDPARITY;
        break;

      case SERIAL_PARITY_EVEN:
        attributes->Parity = EVENPARITY;
        break;

      case SERIAL_PARITY_MARK:
        attributes->Parity = MARKPARITY;
        break;

      case SERIAL_PARITY_SPACE:
        attributes->Parity = SPACEPARITY;
        break;

      default:
        return 0;
    }

    attributes->fParity = TRUE;
  }

  return 1;
}

SerialFlowControl
serialPutFlowControl (SerialAttributes *attributes, SerialFlowControl flow) {
  if (flow & SERIAL_FLOW_OUTPUT_RTS) {
    flow &= ~SERIAL_FLOW_OUTPUT_RTS;
    attributes->fRtsControl = RTS_CONTROL_TOGGLE;
  } else if (flow & SERIAL_FLOW_INPUT_RTS) {
    flow &= ~SERIAL_FLOW_INPUT_RTS;
    attributes->fRtsControl = RTS_CONTROL_HANDSHAKE;
  } else {
    attributes->fRtsControl = RTS_CONTROL_ENABLE;
  }

  if (flow & SERIAL_FLOW_INPUT_XON) {
    flow &= ~SERIAL_FLOW_INPUT_XON;
    attributes->fInX = TRUE;
  } else {
    attributes->fInX = FALSE;
  }

  if (flow & SERIAL_FLOW_OUTPUT_CTS) {
    flow &= ~SERIAL_FLOW_OUTPUT_CTS;
    attributes->fOutxCtsFlow = TRUE;
  } else {
    attributes->fOutxCtsFlow = FALSE;
  }

  if (flow & SERIAL_FLOW_OUTPUT_DSR) {
    flow &= ~SERIAL_FLOW_OUTPUT_DSR;
    attributes->fOutxDsrFlow = TRUE;
  } else {
    attributes->fOutxDsrFlow = FALSE;
  }

  if (flow & SERIAL_FLOW_OUTPUT_XON) {
    flow &= ~SERIAL_FLOW_OUTPUT_XON;
    attributes->fOutX = TRUE;
  } else {
    attributes->fOutX = FALSE;
  }

  return flow;
}

void
serialPutModemState (SerialAttributes *attributes, int enabled) {
  if (enabled) {
    attributes->fDtrControl = DTR_CONTROL_HANDSHAKE;
    attributes->fDsrSensitivity = TRUE;
  } else {
    attributes->fDtrControl = DTR_CONTROL_ENABLE;
    attributes->fDsrSensitivity = FALSE;
  }
}

unsigned int
serialGetDataBits (const SerialAttributes *attributes) {
  return attributes->ByteSize;
}

unsigned int
serialGetStopBits (const SerialAttributes *attributes) {
  if (attributes->StopBits == ONESTOPBIT) return 1;
  if (attributes->StopBits == TWOSTOPBITS) return 2;

  logMessage(LOG_WARNING, "unsupported serial stop bits value: %X", attributes->StopBits);
  return 0;
}

unsigned int
serialGetParityBits (const SerialAttributes *attributes) {
  return (attributes->fParity && (attributes->Parity != NOPARITY))? 1: 0;
}

int
serialCancelInput (SerialDevice *serial) {
  if (PurgeComm(serial->fileHandle, PURGE_RXCLEAR)) return 1;
  logWindowsSystemError("PurgeComm");
  return 0;
}

int
serialCancelOutput (SerialDevice *serial) {
  if (PurgeComm(serial->fileHandle, PURGE_TXCLEAR)) return 1;
  logWindowsSystemError("PurgeComm");
  return 0;
}

int
serialDrainOutput (SerialDevice *serial) {
  if (FlushFileBuffers(serial->fileHandle)) return 1;
  logWindowsSystemError("FlushFileBuffers");
  return 0;
}

int
serialGetAttributes (SerialDevice *serial, SerialAttributes *attributes) {
  attributes->DCBlength = sizeof(serial->currentAttributes);
  if (GetCommState(serial->fileHandle, attributes)) return 1;
  logWindowsSystemError("GetCommState");
  return 0;
}

int
serialPutAttributes (SerialDevice *serial, const SerialAttributes *attributes) {
  if (SetCommState(serial->fileHandle, (SerialAttributes *)attributes)) return 1;
  logWindowsSystemError("SetCommState");
  return 0;
}

