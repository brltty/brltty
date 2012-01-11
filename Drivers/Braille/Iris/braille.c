/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "ascii.h"
#include "cmd.h"
#include "log.h"
#include "parse.h"
#include "scancodes.h"
#include "timing.h"

#define INTERNALSPEED 9600
#define EXTERNALSPEED_EUROBRAILLE 9600
#define EXTERNALSPEED_NATIVE 57600

/* Input/output ports */
#define IRIS_GIO_BASE 0x340
#define IRIS_GIO_INPUT IRIS_GIO_BASE
#define IRIS_GIO_OUTPUT IRIS_GIO_BASE + 1
#define IRIS_GIO_OUTPUT2 IRIS_GIO_BASE + 2
#define DRIVER_LOG_PREFIX "[" STRINGIFY(DRIVER_CODE) "] "
typedef enum {
  PARM_EMBEDDED = 0,
  PARM_LATCHDELAY = 1,
  PARM_PROTOCOL = 2,
} DriverParameter;
#define BRLPARMS "embedded", "latchdelay", "protocol"

typedef enum
{
  IR_PROTOCOL_EUROBRAILLE = 0,
  IR_PROTOCOL_NATIVE = 1,
} Protocol;

#define IR_PROTOCOL_DEFAULT IR_PROTOCOL_EUROBRAILLE

static Protocol protocol = IR_PROTOCOL_DEFAULT;

typedef struct {
  Protocol id;
  const char *name;
  int speed;
} iris_protocol;

const iris_protocol iris_protocols[] = {
  { IR_PROTOCOL_EUROBRAILLE, strtext("eurobraille"), EXTERNALSPEED_EUROBRAILLE },
  { IR_PROTOCOL_NATIVE, strtext("native"), EXTERNALSPEED_NATIVE }
};

#define nb_protocols (  sizeof(iris_protocols) / sizeof(iris_protocol) )

#define BRL_HAVE_PACKET_IO
/* #define BRL_HAVE_VISUAL_DISPLAY */

#include "brl_driver.h"
#include "brldefs-ir.h"

#include "io_generic.h"
#include "system.h"
#include "message.h"

BEGIN_KEY_NAME_TABLE(common)
  KEY_NAME_ENTRY(IR_KEY_L1, "L1"),
  KEY_NAME_ENTRY(IR_KEY_L2, "L2"),
  KEY_NAME_ENTRY(IR_KEY_L3, "L3"),
  KEY_NAME_ENTRY(IR_KEY_L4, "L4"),
  KEY_NAME_ENTRY(IR_KEY_L5, "L5"),
  KEY_NAME_ENTRY(IR_KEY_L6, "L6"),
  KEY_NAME_ENTRY(IR_KEY_L7, "L7"),
  KEY_NAME_ENTRY(IR_KEY_L8, "L8"),

  KEY_NAME_ENTRY(IR_KEY_Menu, "Menu"),
  KEY_NAME_ENTRY(IR_KEY_Z, "Z"),

  KEY_SET_ENTRY(IR_SET_RoutingKeys, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(brl)
  KEY_NAME_ENTRY(IR_KEY_Dot1, "Dot1"),
  KEY_NAME_ENTRY(IR_KEY_Dot2, "Dot2"),
  KEY_NAME_ENTRY(IR_KEY_Dot3, "Dot3"),
  KEY_NAME_ENTRY(IR_KEY_Dot4, "Dot4"),
  KEY_NAME_ENTRY(IR_KEY_Dot5, "Dot5"),
  KEY_NAME_ENTRY(IR_KEY_Dot6, "Dot6"),
  KEY_NAME_ENTRY(IR_KEY_Dot7, "Dot7"),
  KEY_NAME_ENTRY(IR_KEY_Dot8, "Dot8"),
  KEY_NAME_ENTRY(IR_KEY_Backspace, "Backspace"),
  KEY_NAME_ENTRY(IR_KEY_Space, "Space"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLE(pc)
  KEY_SET_ENTRY(IR_SET_Xt, "Xt"),
  KEY_SET_ENTRY(IR_SET_XtE0, "XtE0"),
  KEY_SET_ENTRY(IR_SET_XtE1, "XtE1"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(brl)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(brl),
END_KEY_NAME_TABLES

BEGIN_KEY_NAME_TABLES(pc)
  KEY_NAME_TABLE(common),
  KEY_NAME_TABLE(pc),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(brl)
DEFINE_KEY_TABLE(pc)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(brl),
  &KEY_TABLE_DEFINITION(pc),
END_KEY_TABLE_LIST

#define MAXPACKETSIZE 256

typedef struct {
  const char *name;
  int speed;
  GioEndpoint *gioEndpoint;
  int reading;
  int declaredSize; /* useful when reading Eurobraille packets */
  int prefix;
  unsigned char packet[MAXPACKETSIZE];
  unsigned char *position;
  int waitingForAck;
  struct timeval lastWriteTime;
} Port;

static IrisDeviceType deviceType;
static const KeyTableDefinition *keyTableDefinition;
static int hasVisualDisplay;
static unsigned char *firmwareVersion = NULL;
static unsigned char serialNumber[5] = { 0, 0, 0, 0, 0 };
static int latchDelay;
static unsigned int deviceSleeping = 0;
static unsigned int deviceConnected = 1;
static unsigned int embeddedDriver = 1;
static unsigned int packetForwardMode = 0;
static Port internalPort = {
  .name = "serial:ttyS1",
  .speed = INTERNALSPEED
}, externalPort;
static unsigned char *previousBrailleWindow = NULL;
static int refreshBrailleWindow = 0;
/* static int debugMode = 0; */
static uint32_t linearKeys;

/*
 * Function unused at the moment
static void irDebug(const char *format, ...)
{
  if (debugMode) {
    va_list argp;
    char string[0X100];
    va_start(argp, format);
    vsnprintf(string, sizeof(string), format, argp);
    va_end(argp);
    logMessage(LOG_NOTICE, DRIVER_LOG_PREFIX "%s",string);
  }
}
*/

/*
 * The following debugging functions are unused at the moment.
static void enterDebugMode(void)
{
  debugMode = 1;
  irDebug("Entering debug mode");
}

static void leaveDebugMode(void)
{
  irDebug("Leaving debug mode");
  debugMode = 0;
}

 * End of unused debug functions.
*/

static int openPort(Port *port)
{
  const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = port->speed,
    .parity = SERIAL_PARITY_EVEN
  };

  GioDescriptor gioDescriptor;
  gioInitializeDescriptor(&gioDescriptor);

  gioDescriptor.serial.parameters = &serialParameters;

  if ((port->gioEndpoint = gioConnectResource(port->name, &gioDescriptor))) {
    return 1;
  }

  return 0;
}

static void closePort(Port *port)
{
  if (port->gioEndpoint) {
    gioDisconnectResource (port->gioEndpoint);
    port->gioEndpoint = NULL;
  }
}

static void activateBraille(void)
{
  writePort1(IRIS_GIO_OUTPUT, 0x01);
  usleep(8500);
  writePort1(IRIS_GIO_OUTPUT, 0);
}

static void deactivateBraille(void)
{
  writePort1(IRIS_GIO_OUTPUT, 0x02);
  usleep(8500);
  writePort1(IRIS_GIO_OUTPUT, 0);
}

static int checkLatchState()
{
  static struct timeval startTime;
  static int latchPulled = 0;
  static int elapsedTime = 0;
  unsigned char currentState = readPort1(IRIS_GIO_INPUT) & 0x04;
  if (!latchPulled && !currentState) {
    if (gettimeofday(&startTime, NULL)==-1) {
      logSystemError("gettimeofday");
      return 0;
    }
    latchPulled = 1;
    logMessage(LOG_INFO, DRIVER_LOG_PREFIX "latch pulled");    
    return 0;
  }
  if (latchPulled) {
    int res, ms;
    if (currentState) {
      latchPulled = 0;
      logMessage(LOG_INFO, DRIVER_LOG_PREFIX "latch released");
      elapsedTime = 0;
      return 0;
    }
    ms = millisecondsSince(&startTime);
    logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "latch pulled for %d milliseconds, elapsedTime=%d, latchDelay=%d", ms, elapsedTime, latchDelay);
    if ((elapsedTime<=latchDelay) && (ms>latchDelay)) res = 1;
    else res = 0;
    elapsedTime = ms;
    return res;
  }
  return 0;
}

/* Function readPacket */
/* Returns the size of the read packet. */
/* 0 means no packet has been read and there is no error. */
/* -1 means an error occurred */
static ssize_t readPacket(BrailleDisplay *brl, Port *port, void *packet, size_t size)
{
  unsigned char ch;
  size_t size_;
  while ( gioReadByte (port->gioEndpoint, &ch, port->reading) ) {
    if (port->reading) {
      switch (ch) {
        case DLE:
          if (!port->prefix) {
            port->prefix = 1;
            continue;
          }
        case EOT:
          if (!port->prefix) {
            port->reading = 0;
            size_ = port->position-port->packet;
            if (size_>size) {
              logMessage(LOG_INFO,DRIVER_LOG_PREFIX "Discarding too large packet");
              return 0;
            } else {
              memcpy(packet, port->packet, size_);
              logInputPacket(packet, size_);
              return size_;
            }
          }
        default:
          port->prefix = 0;
          if (port->position-port->packet<MAXPACKETSIZE) {
            *(port->position) = ch; port->position++;
          }
      }
    } else {
      if (ch==SOH) {
        port->reading = 1;
        port->prefix = 0;
        port->position = port->packet;
      } else {
        if ((port->waitingForAck) && (ch==ACK)) {
          port->waitingForAck = 0;
          if (packetForwardMode) {
            char ack = ACK;
            gioWriteData(externalPort.gioEndpoint, &ack, sizeof(ack));
            brl->writeDelay += gioGetMillisecondsToTransfer(externalPort.gioEndpoint, sizeof(ack));
          }
        } else {
          logIgnoredByte(ch);
        }
      }
    }
  }
  if ( errno == EAGAIN )  return 0;
  logSystemError("readPacket");
  return -1;
}

static ssize_t readEurobraillePacket(Port *port, void *packet, size_t size)
{
  unsigned char ch;
  size_t size_;
  while (gioReadByte (port->gioEndpoint, &ch, port->reading)) {
    logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "Got ch=%c(%02x) state=%d", ch, ch, port->reading);
    switch (port->reading)
    {
      case 0:
        if (ch==STX)
        {
          port->reading = 1;
          port->position = port->packet;
          port->declaredSize = 0;
        } else {
          logIgnoredByte(ch);
        };
        break;
      case 1:
        port->declaredSize = ch << 8;
        port->reading = 2;
        break;
      case 2:
        port->declaredSize += ch;
        if (port->declaredSize < 3)
        {
          logMessage(LOG_ERR, DRIVER_LOG_PREFIX "readEuroBraillePacket: invalid declared size %d", port->declaredSize);
          port->reading = 0;
        } else {
          port->declaredSize -= 2;
          if (port->declaredSize > sizeof(port->packet) )
          {
            logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "readEuroBraillePacket: rejecting packet whose declared size is too large");
            port->reading = 0;
            return 0;
          }
          logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "readEuroBraillePacket: declared size = %d", port->declaredSize);
          port->reading = 3;
        };
        break;
      case 3:
        *(port->position) = ch; port->position++;
        if ( (port->position - port->packet) == port->declaredSize) port->reading = 4;
        break;
      case 4:
        port->reading = 0;
        if (ch==ETX) {
          size_ = port->position-port->packet;
          if (size_>size) {
            logMessage(LOG_INFO,DRIVER_LOG_PREFIX "readEurobraillePacket: Discarding too large packet");
            return 0;
          } else {
            memcpy(packet, port->packet, size_);
            return size_;
          }        
        } else {
          logMessage(LOG_INFO,DRIVER_LOG_PREFIX "readEurobraillePacket: Discarding packet whose real size exceeds declared size");
          return 0;
        };
        break;
      default:
        logMessage(LOG_ERR, DRIVER_LOG_PREFIX "readEurobraillePacket: reached unknown state %d", port->reading);
        port->reading = 0;
        break;
    }
  }
  return 0;
}

static inline int needsEscape(unsigned char ch)
{
  static const unsigned char escapedChars[0X20] = {
    [SOH] = 1, [EOT] = 1, [DLE] = 1, [ACK] = 1, [NAK] = 1
  };
  if (ch<sizeof(escapedChars)) return escapedChars[ch];
  else return 0;
}

/* Function writePacket */
/* Returns 1 if the packet is actually written, 0 if the packet is not written */
/* A write can be ignored if the previous packet has not been */
/* acknowledged so far */
static ssize_t writePacket (BrailleDisplay *brl, Port *port, const void *packet, size_t size)
{
  const unsigned char *data = packet;
  unsigned char	buf[2*(size + 1) +3];
  unsigned char *p = buf;
  size_t count;
  ssize_t res;
  int ms = millisecondsSince(&port->lastWriteTime);

  if (port->waitingForAck && (ms > 1000)) {
    logMessage(LOG_WARNING,DRIVER_LOG_PREFIX "Did not receive ACK on port %s",port->name);
    port->waitingForAck = 0;
  }

  if (port->waitingForAck) {
    logMessage(LOG_WARNING,DRIVER_LOG_PREFIX "Did not receive ACK on port %s after %d ms",port->name, ms);
    return 0;
  }

  *p++ = SOH;
  while (size--) {
    if (needsEscape(*data)) *p++ = DLE;
    *p++ = *data++;
  }
  *p++ = EOT;

  count = p - buf;
  logOutputPacket(buf, count);
  brl->writeDelay += gioGetMillisecondsToTransfer(port->gioEndpoint, count);

  res = gioWriteData(port->gioEndpoint, buf, count);
  if (res == -1) {
    logMessage(LOG_WARNING,DRIVER_LOG_PREFIX "in writePacket: gioWriteData failed");
    return -1;
  }

  res = gettimeofday(&port->lastWriteTime, NULL);
  if (res == -11) {
    logMessage(LOG_WARNING,DRIVER_LOG_PREFIX "in writePacket: gettimeofday failed");
    return -1;
  }    

  if (port==&internalPort) port->waitingForAck = 1;
  return count;
}

static ssize_t writeEurobraillePacket (BrailleDisplay *brl, Port *port, const void *packet, size_t size)
{
  int packetSize = size + 2;
  unsigned char	buf[packetSize + 2];
  unsigned char *p = buf;
  *p++ = STX;
  *p++ = (packetSize >> 8) & 0x00FF;
  *p++ = packetSize & 0x00FF;  
  memcpy(p, packet, size);
  buf[sizeof(buf)-1] = ETX;
  gioWriteData(port->gioEndpoint, buf, sizeof(buf));
  brl->writeDelay += gioGetMillisecondsToTransfer(port->gioEndpoint, sizeof(buf));
  gettimeofday(&port->lastWriteTime, NULL);
  return 1;
}

static ssize_t writeEurobraillePacketFromString (BrailleDisplay *brl, Port *port, const char *str)
{
  return writeEurobraillePacket(brl, port, str, strlen(str) + 1);
}

/* Low-level write of dots to the braile display */
/* No check is performed to avoid several consecutive identical writes at this level */
static ssize_t writeDots (BrailleDisplay *brl, Port *port, const unsigned char *dots)
{
  ssize_t size = brl->textColumns * brl->textRows;
  unsigned char packet[IR_MAXWINDOWSIZE+1] = { IR_OPT_WriteBraille };
  unsigned char *p = packet+1;
  int i;
  for (i=0; i<IR_MAXWINDOWSIZE-size; i++) *(p++) = 0; 
  for (i=0; i<size; i++) *(p++) = dots[size-i-1];
  return writePacket(brl, port, packet, sizeof(packet));
}

/* Low-level write of text to the braile display */
/* No check is performed to avoid several consecutive identical writes at this level */
static ssize_t writeWindow (BrailleDisplay *brl, Port *port, const unsigned char *text)
{
  ssize_t size = brl->textColumns * brl->textRows;
  unsigned char dots[size];
  translateOutputCells(dots, text, size);
  return writeDots(brl, port, dots);
}

static void clearWindow(BrailleDisplay *brl, Port *port)
{
  int windowSize = brl->textColumns * brl->textRows;
  unsigned char window[windowSize];
  memset(window, 0, sizeof(window));
  writeWindow(brl, port, window);
}

/*
static void refreshWindow(BrailleDisplay *brl, Port *port)
{
  writeWindow(brl, &internalPort, brl->buffer);
}
*/

static void suspendDevice(BrailleDisplay *brl)
{
  if (!embeddedDriver) return;
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "Suspending device");
  clearWindow(brl, &internalPort);
  usleep(10000);
  closePort(&internalPort);
  internalPort.waitingForAck = 0;
  if (packetForwardMode) {
    static const unsigned char keyPacket[] = { IR_IPT_InteractiveKey, 'Q' };
    writePacket(brl, &externalPort, keyPacket, sizeof(keyPacket));
    closePort(&externalPort);
  }
  deactivateBraille();
  deviceSleeping = 1;
}

static void resumeDevice(BrailleDisplay *brl)
{
  if (!embeddedDriver) return;
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "resuming device");
  deviceSleeping = 0;
  if ( !openPort(&internalPort) )
  {
    logMessage(LOG_WARNING, DRIVER_LOG_PREFIX "openPort failed");
    return;
  }
  activateBraille();
  if (packetForwardMode) {
    static const unsigned char keyPacket[] = { IR_IPT_InteractiveKey, 'W' }; 
    openPort(&externalPort);
    writePacket(brl, &externalPort, keyPacket, sizeof(keyPacket));
  } else refreshBrailleWindow = 1;
}

static ssize_t brl_readPacket (BrailleDisplay *brl, void *packet, size_t size)
{
  if (embeddedDriver && (deviceSleeping || packetForwardMode)) return 0;
  return readPacket(brl, &internalPort, packet, size);
}

/* Function brl_writePacket */
/* Returns 1 if the packet is actually written, 0 if the packet is not written */
static ssize_t brl_writePacket (BrailleDisplay *brl, const void *packet, size_t size)
{
  if (deviceSleeping || packetForwardMode) return 0;
  return writePacket(brl, &internalPort, packet, size);
}

static int brl_reset (BrailleDisplay *brl)
{
  return 0;
}

static void enterPacketForwardMode(BrailleDisplay *brl)
{
  static const unsigned char p[] = { IR_IPT_InteractiveKey, 'Q' };
  char msg[brl->textColumns+1];
  externalPort.speed = iris_protocols[protocol].speed;
  logMessage(LOG_NOTICE, DRIVER_LOG_PREFIX "Entering packet forward mode (port=%s, protocol=%s, speed=%d)", externalPort.name, iris_protocols[protocol].name, externalPort.speed);
  if (!openPort(&externalPort)) return;
  snprintf(msg, sizeof(msg), "%s (%s)", gettext("PC mode"), gettext(iris_protocols[protocol].name));
  message(NULL, msg, MSG_NODELAY);
  if (protocol==IR_PROTOCOL_NATIVE) writePacket(brl, &externalPort, p, sizeof(p));
  packetForwardMode = 1;
}

static void leavePacketForwardMode(BrailleDisplay *brl)
{
  static const unsigned char p[] = { IR_IPT_InteractiveKey, 'Q' };
  logMessage(LOG_NOTICE, DRIVER_LOG_PREFIX "Leaving packet forward mode");
  if (protocol==IR_PROTOCOL_NATIVE) writePacket(brl, &externalPort, p, sizeof(p));
  packetForwardMode = 0;
  closePort(&externalPort);
  refreshBrailleWindow = 1;
}

static int packetToCommand(BrailleDisplay *brl, unsigned char *packet, size_t size)
{
  if (size==2) {
    if (packet[0]==IR_IPT_InteractiveKey) {
      if (packet[1]=='W') {
        logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "Z key pressed");
        enqueueKey(IR_SET_NavigationKeys, IR_KEY_Z);
        return EOF;
      }
      if ((1<=packet[1]) && (packet[1]<=brl->textColumns * brl->textRows)) {
        enqueueKey(IR_SET_RoutingKeys, packet[1]-1);
        return EOF;
      }
    }
  } else if (size==3) {
    if (packet[0]==IR_IPT_XtKeyCode) { /* IrisKB's PC keyboard */
      enqueueXtScanCode(packet[2], packet[1], IR_SET_Xt, IR_SET_XtE0, IR_SET_XtE1);
      return EOF;
    }
    if (packet[0]==IR_IPT_LinearKeys) {
      enqueueUpdatedKeys((packet[1] << 8) | packet[2],
                         &linearKeys, IR_SET_NavigationKeys, IR_KEY_L1);
      return EOF;
    }
    if (packet[0]==IR_IPT_BrailleKeys) {
      enqueueKeys((packet[1] << 8) | packet[2],
                  IR_SET_NavigationKeys, IR_KEY_Dot1);
      return EOF;
    }
  }
  return EOF;
}

static void handleNativePacket(BrailleDisplay *brl, unsigned char *packet1, size_t size)
{
  unsigned char packet2[MAXPACKETSIZE];
  if (size==2) {
    if (packet1[0]==IR_IPT_InteractiveKey) {
      if (packet1[1]=='W') {
        logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "handleNativePacket: discarding Z key");
      } else 
      if ((1<=packet1[1]) && (packet1[1]<=brl->textColumns * brl->textRows)) {
        packet2[0] = 'K';
        packet2[1] = 'I';
        packet2[2] = packet1[1];
        writeEurobraillePacket(brl, &externalPort, packet2, 3);
      }
    }
  } else if (size==3) {
    if (packet1[0]==IR_IPT_XtKeyCode) { /* IrisKB's PC keyboard */
      char description[0x100];
      int command, res;
      unsigned char escape = packet1[1];
      unsigned char key = packet1[2];
      res = xtInterpretScanCode(&command, escape);
      if (res<0)
      {
        logMessage(LOG_ERR, DRIVER_LOG_PREFIX "handleNativePacket: could not interprete escape code 0x%2x", escape);
        return;
      }
      res = xtInterpretScanCode(&command, key);
      if (res<0)
      {
        logMessage(LOG_ERR, DRIVER_LOG_PREFIX "handleNativePacket: could not interprete key code 0x%2x", key);
        return;
      }
      describeCommand(command, description, sizeof(description), 1);
      logMessage(LOG_NOTICE, "handleNativePacket: command: %s", description);
    }
    if (packet1[0]==IR_IPT_LinearKeys) {
      /* enqueueUpdatedKeys((packet[1] << 8) | packet[2],
                         &linearKeys, IR_SET_NavigationKeys, IR_KEY_L1); */
      return;
    }
    if (packet1[0]==IR_IPT_BrailleKeys) {
      /* enqueueKeys((packet[1] << 8) | packet[2],
                  IR_SET_NavigationKeys, IR_KEY_Dot1); */
      return;
    }
  }
}

static void handleEurobraillePacket(BrailleDisplay *brl, const unsigned char *packet, size_t size)
{
  if (size==2 && packet[0]=='S' && packet[1]=='I')
  { /* Send system information */
    char str[256];
    Port *port = &externalPort;
    writeEurobraillePacketFromString(brl, port, "SNIRIS_KB_40");
    writeEurobraillePacketFromString(brl, port, "SHIR4");
    snprintf(str, sizeof(str), "SS%s", serialNumber);
    writeEurobraillePacketFromString(brl, port, str);
    writeEurobraillePacketFromString(brl, port, "SLFR");
    str[0] = 'S'; str[1] = 'G'; str[2] = brl->textColumns;
    writeEurobraillePacket(brl, port, str, 3);
    str[0] = 'S'; str[1] = 'T'; str[2] = 6;
    writeEurobraillePacket(brl, port, str, 3);
    snprintf(str, sizeof(str), "So%d%da", 0xef, 0xf8);
    writeEurobraillePacketFromString(brl, port, str);
    writeEurobraillePacketFromString(brl, port, "SW1.92");
    writeEurobraillePacketFromString(brl, port, "SP1.00 30-10-2006");
    sprintf(str, "SM%d", 0x08);
    writeEurobraillePacketFromString(brl, port, str);
    writeEurobraillePacketFromString(brl, port, "SI");
  } else if (size==brl->textColumns+2 && packet[0]=='B' && packet[1]=='S')
  { /* Write dots to braille display */
    const unsigned char *dots = packet+2;
    writeDots(brl, &internalPort, dots);
  } else {
    logBytes(LOG_WARNING, "handleEurobraillePacket could not handle this packet: ", packet, size);
  }
}

static int readCommand_embedded (BrailleDisplay *brl)
{
  unsigned char packet[MAXPACKETSIZE];
  ssize_t size;
  if (checkLatchState()) {
    if (!deviceSleeping) suspendDevice(brl);
    else {
      resumeDevice(brl);
      return EOF;
    }
  }
  if (deviceSleeping) return BRL_CMD_OFFLINE;

  size = readPacket(brl, &internalPort, packet, sizeof(packet));
  if (size==-1) return BRL_CMD_RESTARTBRL;

  /* The test for Menu key should come first since this key toggles */
  /* packet forward mode on/off */
  if ((size==2) && (packet[0]==IR_IPT_InteractiveKey) && (packet[1]=='Q')) {
    logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "Menu key pressed");
    if (!packetForwardMode) {
      enterPacketForwardMode(brl);
      return BRL_CMD_OFFLINE;
    } else {
      leavePacketForwardMode(brl);
      return EOF;
    }
  }
  if (packetForwardMode) {
    if (size>0) {
      if (protocol==IR_PROTOCOL_NATIVE)
      {
        writePacket(brl, &externalPort, packet, size);
      } else { /* forward using Eurobraille's protocol */
        handleNativePacket(brl, packet, size);
      }
    }
    if (protocol==IR_PROTOCOL_NATIVE)
    { /* Read native packet from external port and forward it to internal port */
      size = readPacket(brl, &externalPort, packet, sizeof(packet));
      if (size>0) writePacket(brl, &internalPort, packet, size);
    } else { /* Read Eurobraille packet from external port and handle it */
      size = readEurobraillePacket(&externalPort, packet, sizeof(packet));
      if (size>0) handleEurobraillePacket(brl, packet, size);
    }
    return BRL_CMD_OFFLINE;
  }
  return packetToCommand(brl, packet, size);
}

static int readCommand_nonembedded (BrailleDisplay *brl)
{
  unsigned char packet[MAXPACKETSIZE];
  ssize_t size;
  size = readPacket(brl, &internalPort, packet, sizeof(packet));
  if (size<0) return BRL_CMD_RESTARTBRL;
  /* The test for Menu key should come first since this key toggles */
  /* packet forward mode on/off */
  if ((size==2) && (packet[0]==IR_IPT_InteractiveKey) && (packet[1]=='Q')) {
    logMessage(LOG_DEBUG,DRIVER_LOG_PREFIX "Menu key pressed");
    if (deviceConnected) {
      deviceConnected = 0;
      return BRL_CMD_OFFLINE;
    }
  }
  if (size>0) {
    if (!deviceConnected) refreshBrailleWindow = 1;
    deviceConnected = 1;
  }

  if (!deviceConnected) return BRL_CMD_OFFLINE;
  
  return packetToCommand(brl, packet, size);
}

static int brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context)
{
  return embeddedDriver ? readCommand_embedded(brl) : readCommand_nonembedded(brl);
}

static int brl_writeWindow (BrailleDisplay *brl, const wchar_t *characters)
{
  const size_t size = brl->textColumns * brl->textRows;

  if (cellsHaveChanged(previousBrailleWindow, brl->buffer, size, NULL, NULL, &refreshBrailleWindow)) {
    ssize_t res = writeWindow(brl, &internalPort, brl->buffer);
    if (res == -1) return 0;
    if (res == 0) refreshBrailleWindow = 1;
  }

  return 1;
}

static void brl_clearWindow(BrailleDisplay *brl)
{
  clearWindow(brl, &internalPort);
}

#ifdef BRL_HAVE_VISUAL_DISPLAY

static void brl_writeVisual(BrailleDisplay *brl)
{
  static unsigned char text[41];
  if (memcmp(text, brl->buffer,40)==0) return;
  memcpy(text, brl->buffer, 40);
  text[40] = '\0';
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "Sending text: %s", text);
}

#endif /* BRL_HAVE_VISUAL_DISPLAY */

static ssize_t sendRequest(BrailleDisplay *brl, IrisOutputPacketType request, unsigned char *response)
{
  unsigned char req = request;
  writePacket(brl, &internalPort, &req, sizeof(req));
  drainBrailleOutput (brl, 100);
  gioAwaitInput(internalPort.gioEndpoint, 1000);
  return readPacket(brl, &internalPort, response, MAXPACKETSIZE);
}

static int brl_construct (BrailleDisplay *brl, char **parameters, const char *device)
{
  int latchDelayMin = 0, latchDelayMax = 10000;
  unsigned char deviceResponse[MAXPACKETSIZE];
  ssize_t size;
  if (!validateYesNo(&embeddedDriver, parameters[PARM_EMBEDDED])) {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "Cannot determine whether driver should be run in embedded mode or not");
    return 0;
  }
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "embeddedDriver=%d",embeddedDriver);
  if (embeddedDriver) {
    const char *protocolChoices[nb_protocols];
    int i;
    for (i=0; i<nb_protocols; i++) protocolChoices[i] = iris_protocols[i].name;
    if (!validateChoice(&protocol, parameters[PARM_PROTOCOL], protocolChoices))
    {
      logMessage(LOG_WARNING, DRIVER_LOG_PREFIX "Invalid value %s of protocol parameter is ignored. Using eurobraille instead.", parameters[PARM_PROTOCOL]);
      protocol = IR_PROTOCOL_DEFAULT;
    } else {
      logMessage(LOG_INFO, DRIVER_LOG_PREFIX "protocol=%s", iris_protocols[protocol].name);
    }
    if (!validateInteger(&latchDelay, parameters[PARM_LATCHDELAY], &latchDelayMin, &latchDelayMax)) latchDelay = IR_DEFAULT_LATCH_DELAY;
    externalPort.name = device;
    if (enablePorts(LOG_ERR, IRIS_GIO_BASE, 3)==-1) {
      logSystemError("ioperm");
      return 0;
    }
    if (!openPort(&internalPort)) return 0;
    activateBraille();
    externalPort.speed = iris_protocols[protocol].speed;
  } else {
    internalPort.name = device;
    internalPort.speed = EXTERNALSPEED_NATIVE;
    if (!openPort(&internalPort)) return 0;
    deviceConnected = 1;
  }
  brl->textRows = 1;
  size = sendRequest(brl, IR_OPT_VersionRequest, deviceResponse);
  if (size <= 0)
  {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "Received no response to version request.");
    closePort(&internalPort);
    return 0;
  }
  if (size < 3)
  {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent a too small response to version request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  if (deviceResponse[0] != IR_IPT_VersionResponse)
  {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent an unexpected response to version request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  hasVisualDisplay = 0;
  switch (deviceResponse[1])
  {
    case 'a':
    case 'A':
      deviceType = IR_DT_KB;
      keyTableDefinition = &KEY_TABLE_DEFINITION(pc);
      brl->textColumns = IR_MAXWINDOWSIZE;
      break;
    case 'l':
    case 'L':
      deviceType = IR_DT_LARGE;
      keyTableDefinition = &KEY_TABLE_DEFINITION(brl);
      brl->textColumns = IR_MAXWINDOWSIZE;
      hasVisualDisplay = 1;
      break;
    case 's':
    case 'S':
      deviceType = IR_DT_SMALL;
      keyTableDefinition = &KEY_TABLE_DEFINITION(brl);
      brl->textColumns = IR_DT_SMALL_WINDOWSIZE;
      break;
    default:
      logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent an invalid device type in response to version request", deviceResponse, size);
      closePort(&internalPort);
      return 0;
  }
  firmwareVersion = malloc(size -1 );
  if (firmwareVersion == NULL)
  {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "brl_construct: could not allocate memory for previous braille window");
    closePort(&internalPort);
    return 0;
  }
  memcpy(firmwareVersion, deviceResponse+2, size-2);
  firmwareVersion[size-2] = 0;
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "The device's firmware version is %s", firmwareVersion);
  size = sendRequest(brl, IR_OPT_SerialNumberRequest, deviceResponse);
  if (size <= 0)
  {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "Received no response to serial number request.");
    closePort(&internalPort);
    return 0;
  }
  if (size != IR_OPT_SERIALNUMBERRESPONSE_LENGTH)
  {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent a response whose length is invalid to serial number request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  if (deviceResponse[0] != IR_IPT_SerialNumberResponse)
  {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent an unexpected response to serial number request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  if (deviceResponse[1] != IR_OPT_SERIALNUMBERRESPONSE_NOWINDOWLENGTH)
  {
    brl->textColumns = deviceResponse[1];
  }
  memcpy(serialNumber, deviceResponse+2, 4);
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "Device's serial number is %s. It has a %s keyboard, a %d-cells braille display and %s visual dipslay.",
             serialNumber,
             keyTableDefinition->bindings,
             brl->textColumns,
             hasVisualDisplay ? "a" : "no" 
  );
  makeOutputTable(dotsTable_ISO11548_1);

  brl->keyBindings = keyTableDefinition->bindings;
  brl->keyNameTables = keyTableDefinition->names;

  previousBrailleWindow = malloc(brl->textColumns * brl->textRows);
  if (previousBrailleWindow==NULL) {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "brl_construct: could not allocate memory for previous braille window");
    closePort(&internalPort);
    return 0;
  }
  linearKeys = 0;
  return 1;
}

static void brl_destruct (BrailleDisplay *brl)
{
  if (embeddedDriver) {
    brl_clearWindow(brl);
    deactivateBraille();
  }
  if (previousBrailleWindow!=NULL) {
    free(previousBrailleWindow);
    previousBrailleWindow = NULL;
  }
  closePort(&internalPort);
}
