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

#include "prologue.h"

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "ascii.h"
#include "cmd.h"
#include "log.h"
#include "parse.h"
#include "timing.h"
#include "system.h"
#include "message.h"


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
  TimePeriod acknowledgementPeriod;
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
  approximateDelay(9);
  writePort1(IRIS_GIO_OUTPUT, 0);
}

static void deactivateBraille(void)
{
  writePort1(IRIS_GIO_OUTPUT, 0x02);
  approximateDelay(9);
  writePort1(IRIS_GIO_OUTPUT, 0);
}

static int checkLatchState()
{
  static TimeValue startTime;
  static int latchPulled = 0;
  static int elapsedTime = 0;

  unsigned char currentState = readPort1(IRIS_GIO_INPUT) & 0x04;

  if (!latchPulled && !currentState) {
    getMonotonicTime(&startTime);
    latchPulled = 1;
    logMessage(LOG_INFO, DRIVER_LOG_PREFIX "latch pulled");    
    return 0;
  }

  if (latchPulled) {
    int res;
    int ms;

    if (currentState) {
      latchPulled = 0;
      logMessage(LOG_INFO, DRIVER_LOG_PREFIX "latch released");
      elapsedTime = 0;
      return 0;
    }

    ms = getMonotonicElapsed(&startTime);
    logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "latch pulled for %d milliseconds, elapsedTime=%d, latchDelay=%d", ms, elapsedTime, latchDelay);

    if ((elapsedTime <= latchDelay) && (ms > latchDelay)) {
      res = 1;
    } else {
      res = 0;
    }

    elapsedTime = ms;
    return res;
  }

  return 0;
}

/* Function readNativePacket */
/* Returns the size of the read packet. */
/* 0 means no packet has been read and there is no error. */
/* -1 means an error occurred */
static size_t readNativePacket(BrailleDisplay *brl, Port *port, void *packet, size_t size)
{
  unsigned char ch;
  size_t size_;
  while ( gioReadByte (port->gioEndpoint, &ch, ( port->reading || port->waitingForAck ) ) ) {
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
          if (packetForwardMode && (protocol == IR_PROTOCOL_NATIVE) ) {
            static const char acknowledgement[] = {ACK};
            writeBraillePacket(brl, externalPort.gioEndpoint, acknowledgement, sizeof(acknowledgement));
          }
        } else {
          logIgnoredByte(ch);
        }
      }
    }
  }
  if ( errno != EAGAIN )  logSystemError("readNativePacket");
  return 0;
}

static size_t readEurobraillePacket(Port *port, void *packet, size_t size)
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

static inline void
startAcknowledgementPeriod (Port *port) {
  startTimePeriod(&port->acknowledgementPeriod, 1000);
}

/* Function writeNativePacket */
/* Returns 1 if the packet is actually written, 0 if the packet is not written */
/* A write can be ignored if the previous packet has not been */
/* acknowledged so far */
static ssize_t writeNativePacket (BrailleDisplay *brl, Port *port, const void *packet, size_t size)
{
  const unsigned char *data = packet;
  unsigned char	buf[2*(size + 1) +3];
  unsigned char *p = buf;
  size_t count;

  if (port->waitingForAck) {
    if (!afterTimePeriod(&port->acknowledgementPeriod, NULL)) {
      errno = EAGAIN;
      return 0;
    }

    logMessage(LOG_WARNING,DRIVER_LOG_PREFIX "Did not receive ACK on port %s",port->name);
    port->waitingForAck = 0;
  }

  *p++ = SOH;
  while (size--) {
    if (needsEscape(*data)) *p++ = DLE;
    *p++ = *data++;
  }
  *p++ = EOT;

  count = p - buf;
  if (!writeBraillePacket(brl, port->gioEndpoint, buf, count)) {
    logMessage(LOG_WARNING,DRIVER_LOG_PREFIX "in writeNativePacket: gioWriteData failed");
    return 0;
  }

  startAcknowledgementPeriod(port);

  if (port==&internalPort) port->waitingForAck = 1;
  return count;
}

/*
static ssize_t
tryWriteNativePacket (BrailleDisplay *brl, Port *port, const void *packet, size_t size) {
  ssize_t res;
  while ( ! (res = writeNativePacket(brl, port, packet, size)) ) {
    if (errno != EAGAIN) return 0;
  }
  return res;
}
*/

static int
writeEurobraillePacket (BrailleDisplay *brl, Port *port, const void *data, size_t size) {
  size_t count;
  size_t packetSize = size + 2;
  unsigned char	packet[packetSize + 2];
  unsigned char *p = packet;

  *p++ = STX;
  *p++ = (packetSize >> 8) & 0x00FF;
  *p++ = packetSize & 0x00FF;  
  p = mempcpy(p, data, size);
  *p++ = ETX;

  count = p - packet;
  if (!writeBraillePacket(brl, port->gioEndpoint, packet, count)) return 0;

  startAcknowledgementPeriod(port);
  return count;
}

typedef struct {
  unsigned char base;
  unsigned char composite;
} CompositeCharacterEntry;

static const CompositeCharacterEntry compositeCharacterTable_circumflex[] = {
  {.base=0X61, .composite=0XE2}, // aâ
  {.base=0X65, .composite=0XEA}, // eê
  {.base=0X69, .composite=0XEE}, // iî
  {.base=0X6F, .composite=0XF4}, // oô
  {.base=0X75, .composite=0XFB}, // uû

  {.base=0X41, .composite=0XC2}, // AÂ
  {.base=0X45, .composite=0XCA}, // EÊ
  {.base=0X49, .composite=0XCE}, // IÎ
  {.base=0X4F, .composite=0XD4}, // OÔ
  {.base=0X55, .composite=0XDB}, // UÛ

  {.base=0X00, .composite=0XA8}
};

static const CompositeCharacterEntry compositeCharacterTable_trema[] = {
  {.base=0X61, .composite=0XE4}, // aä
  {.base=0X65, .composite=0XEB}, // eë
  {.base=0X69, .composite=0XEF}, // iï
  {.base=0X6F, .composite=0XF6}, // oö
  {.base=0X75, .composite=0XFC}, // uü

  {.base=0X41, .composite=0XC4}, // AÄ
  {.base=0X45, .composite=0XCB}, // EË
  {.base=0X49, .composite=0XCF}, // IÏ
  {.base=0X4F, .composite=0XD6}, // OÖ
  {.base=0X55, .composite=0XDC}, // UÜ

  {.base=0X00, .composite=0X5E}
};

static const CompositeCharacterEntry *compositeCharacterTables[] = {
  compositeCharacterTable_circumflex,
  compositeCharacterTable_trema
};

static const CompositeCharacterEntry *compositeCharacterTable;

typedef enum {
  xtsLeftShiftPressed,
  xtsRightShiftPressed,
  xtsShiftLocked,

  xtsLeftControlPressed,
  xtsRightControlPressed,

  xtsLeftAltPressed,
  xtsRightAltPressed,

  xtsLeftWindowsPressed,
  xtsRightWindowsPressed,

  xtsInsertPressed,
  xtsFnPressed
} XtState;

static uint16_t xtState;
#define XTS_BIT(number) (1 << (number))
#define XTS_TEST(bits) (xtState & (bits))
#define XTS_SHIFT XTS_TEST(XTS_BIT(xtsLeftShiftPressed) | XTS_BIT(xtsRightShiftPressed) | XTS_BIT(xtsShiftLocked))
#define XTS_CONTROL XTS_TEST(XTS_BIT(xtsLeftControlPressed) | XTS_BIT(xtsRightControlPressed))
#define XTS_ALT XTS_TEST(XTS_BIT(xtsLeftAltPressed))
#define XTS_ALTGR XTS_TEST(XTS_BIT(xtsRightAltPressed))
#define XTS_WIN XTS_TEST(XTS_BIT(xtsLeftWindowsPressed))
#define XTS_INSERT XTS_TEST(XTS_BIT(xtsInsertPressed))
#define XTS_FN XTS_TEST(XTS_BIT(xtsFnPressed))

typedef enum {
  XtKeyType_ignore = 0, /* required for uninitialized entries */
  XtKeyType_modifier,
  XtKeyType_lock,
  XtKeyType_character,
  XtKeyType_function,
  XtKeyType_complex,
  XtKeyType_composite
} XtKeyType;

typedef struct {
  unsigned char type;
  unsigned char arg1;
  unsigned char arg2;
  unsigned char arg3;
} XtKeyEntry;

typedef enum {
  XT_KEYS_00,
  XT_KEYS_E0,
  XT_KEYS_E1
} XT_KEY_SET;

static const XtKeyEntry *xtCurrentKey;
#define XT_RELEASE 0X80
#define XT_KEY(set,key) ((XT_KEYS_##set << 7) | (key))

static const XtKeyEntry xtKeyTable[] = {
  /* row 1 */
  [XT_KEY(00,0X01)] = { // key 1: escape
    .type = XtKeyType_function,
    .arg1=0X1B
  }
  ,
  [XT_KEY(00,0X3B)] = { // key 2: F1
    .type = XtKeyType_function,
    .arg1=0X70
  }
  ,
  [XT_KEY(00,0X3C)] = { // key 3: F2
    .type = XtKeyType_function,
    .arg1=0X71
  }
  ,
  [XT_KEY(00,0X3D)] = { // key 4: F3
    .type = XtKeyType_function,
    .arg1=0X72
  }
  ,
  [XT_KEY(00,0X3E)] = { // key 5: F4
    .type = XtKeyType_function,
    .arg1=0X73
  }
  ,
  [XT_KEY(00,0X3F)] = { // key 6: F5
    .type = XtKeyType_function,
    .arg1=0X74
  }
  ,
  [XT_KEY(00,0X40)] = { // key 7: F6
    .type = XtKeyType_function,
    .arg1=0X75
  }
  ,
  [XT_KEY(00,0X41)] = { // key 8: F7
    .type = XtKeyType_function,
    .arg1=0X76
  }
  ,
  [XT_KEY(00,0X42)] = { // key 9: F8
    .type = XtKeyType_function,
    .arg1=0X77
  }
  ,
  [XT_KEY(00,0X43)] = { // key 10: F9
    .type = XtKeyType_function,
    .arg1=0X78
  }
  ,
  [XT_KEY(00,0X44)] = { // key 11: F10
    .type = XtKeyType_function,
    .arg1=0X79
  }
  ,
  [XT_KEY(00,0X57)] = { // key 12: F11
    .type = XtKeyType_function,
    .arg1=0X7A
  }
  ,
  [XT_KEY(00,0X58)] = { // key 13: F12
    .type = XtKeyType_function,
    .arg1=0X7B
  }
  ,
  [XT_KEY(00,0X46)] = { // key 14: scroll lock
    .type = XtKeyType_ignore
  }
  ,
  [XT_KEY(E1,0X1D)] = { // key 15: pause break
    .type = XtKeyType_ignore
  }
  ,
  [XT_KEY(E0,0X52)] = { // key 16: insert
    .type = XtKeyType_complex,
    .arg1=0X0F, .arg2=1, .arg3=xtsInsertPressed
  }
  ,
  [XT_KEY(E0,0X53)] = { // key 17: delete
    .type = XtKeyType_function,
    .arg1=0X10, .arg2=1
  }
  ,

  /* row 2 */
  [XT_KEY(00,0X02)] = { // key 1: &1
    .type = XtKeyType_character,
    .arg1=0X26, .arg2=0X31
  }
  ,
  [XT_KEY(00,0X03)] = { // key 2: é2~
    .type = XtKeyType_character,
    .arg1=0XE9, .arg2=0X32, .arg3=0X7E
  }
  ,
  [XT_KEY(00,0X04)] = { // key 3: "3#
    .type = XtKeyType_character,
    .arg1=0X22, .arg2=0X33, .arg3=0X23
  }
  ,
  [XT_KEY(00,0X05)] = { // key 4: '4{
    .type = XtKeyType_character,
    .arg1=0X27, .arg2=0X34, .arg3=0X7B
  }
  ,
  [XT_KEY(00,0X06)] = { // key 5: (5[
    .type = XtKeyType_character,
    .arg1=0X28, .arg2=0X35, .arg3=0X5B
  }
  ,
  [XT_KEY(00,0X07)] = { // key 6: -6|
    .type = XtKeyType_character,
    .arg1=0X2D, .arg2=0X36, .arg3=0X7C
  }
  ,
  [XT_KEY(00,0X08)] = { // key 7: è7`
    .type = XtKeyType_character,
    .arg1=0XE8, .arg2=0X37, .arg3=0X60
  }
  ,
  [XT_KEY(00,0X09)] = { // key 8: _8
    .type = XtKeyType_character,
    .arg1=0X5F, .arg2=0X38, .arg3=0X5C
  }
  ,
  [XT_KEY(00,0X0A)] = { // key 9: ç9^
    .type = XtKeyType_character,
    .arg1=0XE7, .arg2=0X39, .arg3=0X5E
  }
  ,
  [XT_KEY(00,0X0B)] = { // key 10: à0@
    .type = XtKeyType_character,
    .arg1=0XE0, .arg2=0X30, .arg3=0X40
  }
  ,
  [XT_KEY(00,0X0C)] = { // key 11: )°]
    .type = XtKeyType_character,
    .arg1=0X29, .arg2=0XB0, .arg3=0X5D
  }
  ,
  [XT_KEY(00,0X0D)] = { // key 12: =+}
    .type = XtKeyType_character,
    .arg1=0X3D, .arg2=0X2B, .arg3=0X7D
  }
  ,
  [XT_KEY(00,0X29)] = { // key 13: ²
    .type = XtKeyType_character,
    .arg1=0XB2
  }
  ,
  [XT_KEY(00,0X0E)] = { // key 14: backspace
    .type = XtKeyType_function,
    .arg1=0X08
  }
  ,

  /* row 3 */
  [XT_KEY(00,0X0F)] = { // key 1: tab
    .type = XtKeyType_function,
    .arg1=0X09
  }
  ,
  [XT_KEY(00,0X10)] = { // key 2: aA
    .type = XtKeyType_character,
    .arg1=0X61, .arg2=0X41
  }
  ,
  [XT_KEY(00,0X11)] = { // key 3: zZ
    .type = XtKeyType_character,
    .arg1=0X7A, .arg2=0X5A
  }
  ,
  [XT_KEY(00,0X12)] = { // key 4: eE
    .type = XtKeyType_character,
    .arg1=0X65, .arg2=0X45, .arg3=0X80
  }
  ,
  [XT_KEY(00,0X13)] = { // key 5: rR®
    .type = XtKeyType_character,
    .arg1=0X72, .arg2=0X52, .arg3=0XAE
  }
  ,
  [XT_KEY(00,0X14)] = { // key 6: tT
    .type = XtKeyType_character,
    .arg1=0X74, .arg2=0X54, .arg3=0X99
  }
  ,
  [XT_KEY(00,0X15)] = { // key 7: yY
    .type = XtKeyType_character,
    .arg1=0X79, .arg2=0X59
  }
  ,
  [XT_KEY(00,0X16)] = { // key 8: uU
    .type = XtKeyType_character,
    .arg1=0X75, .arg2=0X55
  }
  ,
  [XT_KEY(00,0X17)] = { // key 9: iI
    .type = XtKeyType_character,
    .arg1=0X69, .arg2=0X49
  }
  ,
  [XT_KEY(00,0X18)] = { // key 10: oO
    .type = XtKeyType_character,
    .arg1=0X6F, .arg2=0X4F
  }
  ,
  [XT_KEY(00,0X19)] = { // key 11: pP
    .type = XtKeyType_character,
    .arg1=0X70, .arg2=0X50
  }
  ,
  [XT_KEY(00,0X1A)] = { // key 12: circumflex tréma
    .type = XtKeyType_composite,
    .arg1=1, .arg2=2
  }
  ,
  [XT_KEY(00,0X1B)] = { // key 13: $£¤
    .type = XtKeyType_character,
    .arg1=0X24, .arg2=0XA3, .arg3=0XA4
  }
  ,
  [XT_KEY(00,0X1C)] = { // key 14: return
    .type = XtKeyType_function,
    .arg1=0X0D
  }
  ,

  /* row 4 */
  [XT_KEY(00,0X3A)] = { // key 1: shift lock
    .type = XtKeyType_lock,
    .arg1=xtsShiftLocked
  }
  ,
  [XT_KEY(00,0X1E)] = { // key 2: qQ
    .type = XtKeyType_character,
    .arg1=0X71, .arg2=0X51
  }
  ,
  [XT_KEY(00,0X1F)] = { // key 3: sS
    .type = XtKeyType_character,
    .arg1=0X73, .arg2=0X53
  }
  ,
  [XT_KEY(00,0X20)] = { // key 4: dD
    .type = XtKeyType_character,
    .arg1=0X64, .arg2=0X44
  }
  ,
  [XT_KEY(00,0X21)] = { // key 5: fF
    .type = XtKeyType_character,
    .arg1=0X66, .arg2=0X46
  }
  ,
  [XT_KEY(00,0X22)] = { // key 6: gG
    .type = XtKeyType_character,
    .arg1=0X67, .arg2=0X47
  }
  ,
  [XT_KEY(00,0X23)] = { // key 7: hH
    .type = XtKeyType_character,
    .arg1=0X68, .arg2=0X48
  }
  ,
  [XT_KEY(00,0X24)] = { // key 8: jJ
    .type = XtKeyType_character,
    .arg1=0X6A, .arg2=0X4A
  }
  ,
  [XT_KEY(00,0X25)] = { // key 9: kK
    .type = XtKeyType_character,
    .arg1=0X6B, .arg2=0X4B
  }
  ,
  [XT_KEY(00,0X26)] = { // key 10: lL
    .type = XtKeyType_character,
    .arg1=0X6C, .arg2=0X4C
  }
  ,
  [XT_KEY(00,0X27)] = { // key 11: mM
    .type = XtKeyType_character,
    .arg1=0X6D, .arg2=0X4D
  }
  ,
  [XT_KEY(00,0X28)] = { // key 12: ù%
    .type = XtKeyType_character,
    .arg1=0XF9, .arg2=0X25
  }
  ,
  [XT_KEY(00,0X2B)] = { // key 13: *µ
    .type = XtKeyType_character,
    .arg1=0X2A, .arg2=0XB5
  }
  ,
  [XT_KEY(00,0X1C)] = { // key 14: return
    .type = XtKeyType_function,
    .arg1=0X0D
  }
  ,

  /* row 5 */
  [XT_KEY(00,0X2A)] = { // key 1: left shift
    .type = XtKeyType_modifier,
    .arg1=xtsLeftShiftPressed, .arg2=xtsShiftLocked
  }
  ,
  [XT_KEY(00,0X2C)] = { // key 2: wW
    .type = XtKeyType_character,
    .arg1=0X77, .arg2=0X57
  }
  ,
  [XT_KEY(00,0X2D)] = { // key 3: xX
    .type = XtKeyType_character,
    .arg1=0X78, .arg2=0X58
  }
  ,
  [XT_KEY(00,0X2E)] = { // key 4: cC©
    .type = XtKeyType_character,
    .arg1=0X63, .arg2=0X43, .arg3=0XA9
  }
  ,
  [XT_KEY(00,0X2F)] = { // key 5: vV
    .type = XtKeyType_character,
    .arg1=0X76, .arg2=0X56
  }
  ,
  [XT_KEY(00,0X30)] = { // key 6: bB
    .type = XtKeyType_character,
    .arg1=0X62, .arg2=0X42
  }
  ,
  [XT_KEY(00,0X31)] = { // key 7: nN
    .type = XtKeyType_character,
    .arg1=0X6E, .arg2=0X4E
  }
  ,
  [XT_KEY(00,0X32)] = { // key 8: ,?
    .type = XtKeyType_character,
    .arg1=0X2C, .arg2=0X3F
  }
  ,
  [XT_KEY(00,0X33)] = { // key 9: ;.
    .type = XtKeyType_character,
    .arg1=0X3B, .arg2=0X2E
  }
  ,
  [XT_KEY(00,0X34)] = { // key 10: :/
    .type = XtKeyType_character,
    .arg1=0X3A, .arg2=0X2F
  }
  ,
  [XT_KEY(00,0X35)] = { // key 11: !§
    .type = XtKeyType_character,
    .arg1=0X21, .arg2=0XA7
  }
  ,
  [XT_KEY(00,0X56)] = { // key 12: <>
    .type = XtKeyType_character,
    .arg1=0X3C, .arg2=0X3E
  }
  ,
  [XT_KEY(00,0X36)] = { // key 13: right shift
    .type = XtKeyType_modifier,
    .arg1=xtsRightShiftPressed, .arg2=xtsShiftLocked
  }
  ,

  /* row 6 */
  [XT_KEY(00,0X1D)] = { // key 1: left control
    .type = XtKeyType_modifier,
    .arg1=xtsLeftControlPressed
  }
  ,
  [XT_KEY(E1,0X01)] = { // key 2: fn
    .type = XtKeyType_modifier,
    .arg1=xtsFnPressed
  }
  ,
  [XT_KEY(E0,0X5B)] = { // key 3: left windows
    .type = XtKeyType_complex,
    .arg1=0X5B, .arg3=xtsLeftWindowsPressed
  }
  ,
  [XT_KEY(00,0X38)] = { // key 4: left alt
    .type = XtKeyType_modifier,
    .arg1=xtsLeftAltPressed
  }
  ,
  [XT_KEY(00,0X39)] = { // key 5: space
    .type = XtKeyType_function,
    .arg1=0X20
  }
  ,
  [XT_KEY(E0,0X38)] = { // key 6: right alt
    .type = XtKeyType_modifier,
    .arg1=xtsRightAltPressed
  }
  ,
  [XT_KEY(E0,0X5D)] = { // key 7: right windows
    .type = XtKeyType_function,
    .arg1=0X5D
  }
  ,
  [XT_KEY(E0,0X1D)] = { // key 8: right control
    .type = XtKeyType_modifier,
    .arg1=xtsRightControlPressed
  }
  ,

  /* arrow keys */
  [XT_KEY(E0,0X48)] = { // key 1: up arrow
    .type = XtKeyType_function,
    .arg1=0X0D, .arg2=1
  }
  ,
  [XT_KEY(E0,0X4B)] = { // key 2: left arrow
    .type = XtKeyType_function,
    .arg1=0X0B, .arg2=1
  }
  ,
  [XT_KEY(E0,0X50)] = { // key 3: down arrow
    .type = XtKeyType_function,
    .arg1=0X0E, .arg2=1
  }
  ,
  [XT_KEY(E0,0X4D)] = { // key 4: right arrow
    .type = XtKeyType_function,
    .arg1=0X0C, .arg2=1
  }
  ,
  [XT_KEY(E0,0X49)] = { // fn + key 1: page up
    .type = XtKeyType_function,
    .arg1=0X09, .arg2=1
  }
  ,
  [XT_KEY(E0,0X47)] = { // fn + key 2: home
    .type = XtKeyType_function,
    .arg1=0X07, .arg2=1
  }
  ,
  [XT_KEY(E0,0X51)] = { // fn + key 3: page down
    .type = XtKeyType_function,
    .arg1=0X0A, .arg2=1
  }
  ,
  [XT_KEY(E0,0X4F)] = { // fn + key 4: end
    .type = XtKeyType_function,
    .arg1=0X08, .arg2=1
  }
};

static int
writeEurobrailleStringPacket (BrailleDisplay *brl, Port *port, const char *string) {
  return writeEurobraillePacket(brl, port, string, strlen(string) + 1);
}

/* Low-level write of dots to the braile display */
/* No check is performed to avoid several consecutive identical writes at this level */
static ssize_t writeDots (BrailleDisplay *brl, Port *port, const unsigned char *dots)
{
  size_t size = brl->textColumns * brl->textRows;
  unsigned char packet[IR_MAXWINDOWSIZE+1] = { IR_OPT_WriteBraille };
  unsigned char *p = packet+1;
  int i;
  for (i=0; i<IR_MAXWINDOWSIZE-size; i++) *(p++) = 0; 
  for (i=0; i<size; i++) *(p++) = dots[size-i-1];
  return writeNativePacket(brl, port, packet, sizeof(packet));
}

/* Low-level write of text to the braile display */
/* No check is performed to avoid several consecutive identical writes at this level */
static ssize_t writeWindow (BrailleDisplay *brl, Port *port, const unsigned char *text)
{
  size_t size = brl->textColumns * brl->textRows;
  unsigned char dots[size];
  translateOutputCells(dots, text, size);
  return writeDots(brl, port, dots);
}

static int clearWindow(BrailleDisplay *brl, Port *port)
{
  int windowSize = brl->textColumns * brl->textRows;
  unsigned char window[windowSize];
  memset(window, 0, sizeof(window));
  return writeWindow(brl, port, window);
}

/*
static void refreshWindow(BrailleDisplay *brl, Port *port)
{
  writeWindow(brl, &internalPort, brl->buffer);
}
*/

static int suspendDevice(BrailleDisplay *brl)
{
  if (!embeddedDriver) return 1;
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "Suspending device");
  if (packetForwardMode) {
    static const unsigned char keyPacket[] = { IR_IPT_InteractiveKey, 'Q' };
    if ( ! writeNativePacket(brl, &externalPort, keyPacket, sizeof(keyPacket)) ) return 0;
    closePort(&externalPort);
  }
  while (! clearWindow(brl, &internalPort)) {
    if (errno != EAGAIN) return 0;
  }
  approximateDelay(10);
  closePort(&internalPort);
  internalPort.waitingForAck = 0;

  deactivateBraille();
  deviceSleeping = 1;
  return 1;
}

static int resumeDevice(BrailleDisplay *brl)
{
  if (!embeddedDriver) return 1;
  logMessage(LOG_INFO, DRIVER_LOG_PREFIX "resuming device");
  deviceSleeping = 0;
  if ( !openPort(&internalPort) )
  {
    logMessage(LOG_WARNING, DRIVER_LOG_PREFIX "openPort failed");
    return 0;
  }
  activateBraille();
  if (packetForwardMode) {
    static const unsigned char keyPacket[] = { IR_IPT_InteractiveKey, 'W' }; 
    if (! openPort(&externalPort) ) return 0;
    if (! writeNativePacket(brl, &externalPort, keyPacket, sizeof(keyPacket)) ) return 0;
  } else refreshBrailleWindow = 1;
  return 1;
}

static ssize_t brl_readPacket (BrailleDisplay *brl, void *packet, size_t size)
{
  if (embeddedDriver && (deviceSleeping || packetForwardMode)) return 0;
  return readNativePacket(brl, &internalPort, packet, size);
}

/* Function brl_writePacket */
/* Returns 1 if the packet is actually written, 0 if the packet is not written */
static ssize_t brl_writePacket (BrailleDisplay *brl, const void *packet, size_t size)
{
  if (deviceSleeping || packetForwardMode) {
    errno = EAGAIN;
    return 0;
  }
  return writeNativePacket(brl, &internalPort, packet, size);
}

static int brl_reset (BrailleDisplay *brl)
{
  return 0;
}

static int enterPacketForwardMode(BrailleDisplay *brl)
{
  logMessage(LOG_NOTICE, DRIVER_LOG_PREFIX "Entering packet forward mode (port=%s, protocol=%s, speed=%d)", externalPort.name, iris_protocols[protocol].name, externalPort.speed);

  externalPort.speed = iris_protocols[protocol].speed;
  if (!openPort(&externalPort)) return 0;

  {
    char msg[brl->textColumns+1];

    snprintf(msg, sizeof(msg), "%s (%s)", gettext("PC mode"), gettext(iris_protocols[protocol].name));
    message(NULL, msg, MSG_NODELAY);
  }

  switch (protocol) {
    case IR_PROTOCOL_NATIVE: {
      static const unsigned char p[] = { IR_IPT_InteractiveKey, 'Q' };

      if (! writeNativePacket(brl, &externalPort, p, sizeof(p))) return 0;
      break;
    }

    case IR_PROTOCOL_EUROBRAILLE:
      compositeCharacterTable = NULL;
      xtCurrentKey = NULL;
      xtState = 0;
      break;
  }

  packetForwardMode = 1;
  return 1;
}

static int leavePacketForwardMode(BrailleDisplay *brl)
{
  static const unsigned char p[] = { IR_IPT_InteractiveKey, 'Q' };
  logMessage(LOG_NOTICE, DRIVER_LOG_PREFIX "Leaving packet forward mode");
  if (protocol==IR_PROTOCOL_NATIVE) {
    if (! writeNativePacket(brl, &externalPort, p, sizeof(p)) ) return 0;
  }

  packetForwardMode = 0;
  closePort(&externalPort);
  refreshBrailleWindow = 1;
  return 1;
}

typedef struct {
  int (*handleZKey) (BrailleDisplay *brl, Port *port);
  int (*handleRoutingKey) (BrailleDisplay *brl, Port *port, unsigned char key);
  int (*handlePCKey) (BrailleDisplay *brl, Port *port, int repeat, unsigned char escape, unsigned char key);
  int (*handleFunctionKeys) (BrailleDisplay *brl, Port *port, uint16_t keys);
  int (*handleBrailleKeys) (BrailleDisplay *brl, Port *port, unsigned int keys);
} KeyHandlers;

static int
core_handleZKey(BrailleDisplay *brl, Port *port) {
  logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "Z key pressed");
  /* return enqueueKey(IR_SET_NavigationKeys, IR_KEY_Z); */
  protocol = (protocol == IR_PROTOCOL_EUROBRAILLE) ?
    IR_PROTOCOL_NATIVE : IR_PROTOCOL_EUROBRAILLE;
  return 1;
}

static int
core_handleRoutingKey(BrailleDisplay *brl, Port *port, unsigned char key) {
  return enqueueKey(IR_SET_RoutingKeys, key-1);
}

static int
core_handlePCKey(BrailleDisplay *brl, Port *port, int repeat, unsigned char escape, unsigned char code) {
  return enqueueXtScanCode(code, escape, IR_SET_Xt, IR_SET_XtE0, IR_SET_XtE1);
}

static int
core_handleFunctionKeys(BrailleDisplay *brl, Port *port, uint16_t keys) {
  return enqueueUpdatedKeys(keys, &linearKeys, IR_SET_NavigationKeys, IR_KEY_L1);
}

static int
core_handleBrailleKeys(BrailleDisplay *brl, Port *port, unsigned int keys) {
  return enqueueKeys(keys, IR_SET_NavigationKeys, IR_KEY_Dot1);
}

static const KeyHandlers coreKeyHandlers = {
  .handleZKey = core_handleZKey,
  .handleRoutingKey = core_handleRoutingKey,
  .handlePCKey = core_handlePCKey,
  .handleFunctionKeys = core_handleFunctionKeys,
  .handleBrailleKeys = core_handleBrailleKeys
};

static int
eurobrl_handleZKey(BrailleDisplay *brl, Port *port) {
  logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "eurobrl_handleZKey: discarding Z key");
  return 1;
}

static int
eurobrl_handleRoutingKey(BrailleDisplay *brl, Port *port, unsigned char key) {
  unsigned char data[] = {
    0X4B, 0X49, 1, key
  };
  return writeEurobraillePacket(brl, port, data, sizeof(data));
}

static int
eurobrl_handlePCKey(BrailleDisplay *brl, Port *port, int repeat, unsigned char escape, unsigned char key) {
  unsigned char data[] = {0X4B, 0X5A, 0, 0, 0, 0};
  const XtKeyEntry *xke = &xtKeyTable[key & ~XT_RELEASE];

  switch (escape) {
    case 0XE0:
      xke += XT_KEY(E0, 0);
      break;

    case 0XE1:
      xke += XT_KEY(E1, 0);
      break;

    default:
    case 0X00:
      xke += XT_KEY(00, 0);
      break;
  }

  if (xke >= (xtKeyTable + ARRAY_COUNT(xtKeyTable))) {
    static const XtKeyEntry xtKeyEntry = {
      .type = XtKeyType_ignore
    };

    xke = &xtKeyEntry;
  }

  if (key & XT_RELEASE) {
    int current = xke == xtCurrentKey;
    xtCurrentKey = NULL;

    switch (xke->type) {
      case XtKeyType_modifier:
        xtState &= ~XTS_BIT(xke->arg1);
        return 1;

      case XtKeyType_complex:
        xtState &= ~XTS_BIT(xke->arg3);
        if (current) goto isFunction;
        return 1;

      default:
        return 1;
    }
  } else {
    xtCurrentKey = xke;

    switch (xke->type) {
      case XtKeyType_modifier:
        xtState |= XTS_BIT(xke->arg1);
        xtState &= ~XTS_BIT(xke->arg2);
        return 1;

      case XtKeyType_complex:
        xtState |= XTS_BIT(xke->arg3);
        return 1;

      case XtKeyType_lock:
        xtState |= XTS_BIT(xke->arg1);
        return 1;

      case XtKeyType_character:
        if (xke->arg3 && XTS_ALTGR) {
          data[5] = xke->arg3;
        } else if (xke->arg2 && XTS_SHIFT) {
          data[5] = xke->arg2;
        } else {
          data[5] = xke->arg1;
        }
        break;

      case XtKeyType_function:
      isFunction:
        data[3] = xke->arg1;
        data[2] = xke->arg2;
        break;

      case XtKeyType_composite: {
        unsigned char index;

        if (xke->arg2 && XTS_SHIFT) {
          index = xke->arg2;
        } else {
          index = xke->arg1;
        }

        if (index) compositeCharacterTable = compositeCharacterTables[index - 1];
        return 1;
      }

      default:
        return 1;
    }
  }

  if (XTS_TEST(XTS_BIT(xtsLeftShiftPressed) | XTS_BIT(xtsRightShiftPressed))) data[4] |= 0X01;
  if (XTS_CONTROL) data[4] |= 0X02;
  if (XTS_ALT) data[4] |= 0X04;
  if (XTS_TEST(XTS_BIT(xtsShiftLocked))) data[4] |= 0X08;
  if (XTS_WIN) data[4] |= 0X10;
  if (XTS_ALTGR) data[4] |= 0X20;
  if (XTS_INSERT) data[4] |= 0X80;

  if (compositeCharacterTable) {
    unsigned char *byte = &data[5];

    if (*byte) {
      const CompositeCharacterEntry *cce = compositeCharacterTable;

      while (cce->base) {
        if (cce->base == *byte) {
          *byte = cce->composite;
          break;
        }

        cce += 1;
      }

      if (!cce->base && cce->composite) {
        unsigned char original = *byte;
        *byte = cce->composite;
        if (!writeEurobraillePacket(brl, port, data, sizeof(data))) return 0;
        *byte = original;
      }
    }

    compositeCharacterTable = NULL;
  }

  return writeEurobraillePacket(brl, port, data, sizeof(data));
}

static int
eurobrl_handleFunctionKeys(BrailleDisplay *brl, Port *port, uint16_t keys) {
  if (keys) {
    unsigned char data[] = {
      0X4B, 0X43, 0, (
        (keys & 0XF) |
        ((keys >> 1) & 0XF0)
      )
    };

    if (!writeEurobraillePacket(brl, port, data, sizeof(data))) return 0;
  }

  return 1;
}

static int
eurobrl_handleBrailleKeys(BrailleDisplay *brl, Port *port, unsigned int keys) {
  unsigned char data[] = {
    0X4B, 0X42,
    (keys >> 8) & 0XFF,
    keys & 0XFF
  };
  return writeEurobraillePacket(brl, port, data, sizeof(data));
}

static const KeyHandlers eurobrailleKeyHandlers = {
  .handleZKey = eurobrl_handleZKey,
  .handleRoutingKey = eurobrl_handleRoutingKey,
  .handlePCKey = eurobrl_handlePCKey,
  .handleFunctionKeys = eurobrl_handleFunctionKeys,
  .handleBrailleKeys = eurobrl_handleBrailleKeys
};

static int
handleNativePacket (BrailleDisplay *brl, Port *port, const KeyHandlers *keyHandlers, unsigned char *packet, size_t size) {
  if (size == 2) {
    if (packet[0] == IR_IPT_InteractiveKey) {
      if (packet[1] == 'W') {
        return keyHandlers->handleZKey(brl, port);
      }

      if ((1 <= packet[1]) && (packet[1] <= (brl->textColumns * brl->textRows))) {
        return keyHandlers->handleRoutingKey(brl, port, packet[1]);
      }
    }
  } else if (size == 3) {
    int repeat = (packet[0] == IR_IPT_XtKeyCodeRepeat);
    if ((packet[0] == IR_IPT_XtKeyCode) || repeat) {
      return keyHandlers->handlePCKey(brl, port, repeat, packet[1], packet[2]);
    }

    if (packet[0] == IR_IPT_LinearKeys) {
      uint16_t keys = (packet[1] << 8) | packet[2];
      return keyHandlers->handleFunctionKeys(brl, port, keys);
    }

    if (packet[0] == IR_IPT_BrailleKeys) {
      unsigned int keys = (packet[1] << 8) | packet[2];
      return keyHandlers->handleBrailleKeys(brl, port, keys);
    }
  }
  logBytes(LOG_WARNING, "unhandled Iris packet", packet, size);
  return 0;
}

static void handleEurobraillePacket(BrailleDisplay *brl, const unsigned char *packet, size_t size)
{
  if (size==2 && packet[0]=='S' && packet[1]=='I')
  { /* Send system information */
    char str[256];
    Port *port = &externalPort;
    writeEurobrailleStringPacket(brl, port, "SNIRIS_KB_40");
    writeEurobrailleStringPacket(brl, port, "SHIR4");
    snprintf(str, sizeof(str), "SS%s", serialNumber);
    writeEurobrailleStringPacket(brl, port, str);
    writeEurobrailleStringPacket(brl, port, "SLFR");
    str[0] = 'S'; str[1] = 'G'; str[2] = brl->textColumns;
    writeEurobraillePacket(brl, port, str, 3);
    str[0] = 'S'; str[1] = 'T'; str[2] = 6;
    writeEurobraillePacket(brl, port, str, 3);
    snprintf(str, sizeof(str), "So%d%da", 0xef, 0xf8);
    writeEurobrailleStringPacket(brl, port, str);
    writeEurobrailleStringPacket(brl, port, "SW1.92");
    writeEurobrailleStringPacket(brl, port, "SP1.00 30-10-2006");
    snprintf(str, sizeof(str), "SM%d", 0x08);
    writeEurobrailleStringPacket(brl, port, str);
    writeEurobrailleStringPacket(brl, port, "SI");
  } else if (size==brl->textColumns+2 && packet[0]=='B' && packet[1]=='S')
  { /* Write dots to braille display */
    const unsigned char *dots = packet+2;
    writeDots(brl, &internalPort, dots);
  } else {
    logBytes(LOG_WARNING, "handleEurobraillePacket could not handle this packet: ", packet, size);
  }
}

static inline int
isMenuKey (const unsigned char *packet, size_t size) {
  return (size == 2) && (packet[0] == IR_IPT_InteractiveKey) && (packet[1] == 'Q');
}

static int readCommand_embedded (BrailleDisplay *brl)
{
  unsigned char packet[MAXPACKETSIZE];
  size_t size;

  if (checkLatchState()) {
    if (deviceSleeping) {
      if (! resumeDevice(brl) ) goto failure;
      return EOF;
    }

    if (! suspendDevice(brl) ) goto failure;
  }

  if (deviceSleeping) return BRL_CMD_OFFLINE;

  while ((size = readNativePacket(brl, &internalPort, packet, sizeof(packet)))) {
    /* The test for Menu key should come first since this key toggles */
    /* packet forward mode on/off */
    if (isMenuKey(packet, size)) {
      logMessage(LOG_DEBUG, DRIVER_LOG_PREFIX "Menu key pressed");
      if (packetForwardMode) {
        if (! leavePacketForwardMode(brl) ) goto failure;
        continue;
      } else {
        if (! enterPacketForwardMode(brl) ) goto failure;
        break;
      }
    }

    if (!packetForwardMode) {
      handleNativePacket(brl, NULL, &coreKeyHandlers, packet, size);
    } else if (protocol == IR_PROTOCOL_NATIVE) {
      if (! writeNativePacket(brl, &externalPort, packet, size)) goto failure;
    } else {
      handleNativePacket(brl, &externalPort, &eurobrailleKeyHandlers, packet, size);
    }
  }

  if ( errno != EAGAIN )  goto failure;

  if (packetForwardMode) {
    if (protocol == IR_PROTOCOL_NATIVE) {
      while ((size = readNativePacket(brl, &externalPort, packet, sizeof(packet)))) {
        if (! writeNativePacket(brl, &internalPort, packet, size) ) goto failure;
      }
    } else { /* Read Eurobraille packet from external port and handle it */
      while ((size = readEurobraillePacket(&externalPort, packet, sizeof(packet)))) {
        handleEurobraillePacket(brl, packet, size);
      }
    }

    return BRL_CMD_OFFLINE;
  }

  return EOF;

failure:
  logSystemError("readCommand_embedded");
  return BRL_CMD_RESTARTBRL;
}

static int readCommand_nonembedded (BrailleDisplay *brl)
{
  unsigned char packet[MAXPACKETSIZE];
  size_t size;

  while ( (size = readNativePacket(brl, &internalPort, packet, sizeof(packet)) )) {
    /* The test for Menu key should come first since this key toggles */
    /* packet forward mode on/off */
    if (isMenuKey(packet, size)) {
      logMessage(LOG_DEBUG,DRIVER_LOG_PREFIX "Menu key pressed");
      if (deviceConnected) {
        deviceConnected = 0;
        return BRL_CMD_OFFLINE;
      }
    }

    if (!deviceConnected) {
      refreshBrailleWindow = 1;
      deviceConnected = 1;
    }
    
    handleNativePacket(brl, NULL, &coreKeyHandlers, packet, size);
  }

  if (errno != EAGAIN) return BRL_CMD_RESTARTBRL;  
  if (!deviceConnected) return BRL_CMD_OFFLINE;
  return EOF;
}

static int (*readCommand)(BrailleDisplay *brl) = NULL;

static int brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context)
{
  return readCommand(brl);
}

static int brl_writeWindow (BrailleDisplay *brl, const wchar_t *characters)
{
  const size_t size = brl->textColumns * brl->textRows;

  if (cellsHaveChanged(previousBrailleWindow, brl->buffer, size, NULL, NULL, &refreshBrailleWindow)) {
    ssize_t res = writeWindow(brl, &internalPort, brl->buffer);

    if (!res) {
      if (errno != EAGAIN) return 0;
      refreshBrailleWindow = 1;
    }
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

static ssize_t askDevice(BrailleDisplay *brl, IrisOutputPacketType request, unsigned char *response, size_t size)
{
  {
    const unsigned char data[] = {request};
    if (! writeNativePacket(brl, &internalPort, data, sizeof(data)) ) return 0;
    drainBrailleOutput(brl, 0);
  }

  while (gioAwaitInput(internalPort.gioEndpoint, 1000)) {
    size_t res = readNativePacket(brl, &internalPort, response, size);
    if (res) return res;
    if (errno != EAGAIN) break;
  }

  return 0;
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
    readCommand = &readCommand_embedded;
  } else {
    internalPort.name = device;
    internalPort.speed = EXTERNALSPEED_NATIVE;
    if (!openPort(&internalPort)) return 0;
    deviceConnected = 1;
    readCommand = &readCommand_nonembedded;
  }
  brl->textRows = 1;
  if ( ! (size = askDevice(brl, IR_OPT_VersionRequest, deviceResponse, sizeof(deviceResponse)) )) {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "Received no response to version request.");
    closePort(&internalPort);
    return 0;
  }
  if (size < 3) {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent a too small response to version request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  if (deviceResponse[0] != IR_IPT_VersionResponse) {
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
  if ( ! ( size = askDevice(brl, IR_OPT_SerialNumberRequest, deviceResponse, sizeof(deviceResponse)) )) {
    logMessage(LOG_ERR, DRIVER_LOG_PREFIX "Received no response to serial number request.");
    closePort(&internalPort);
    return 0;
  }
  if (size != IR_OPT_SERIALNUMBERRESPONSE_LENGTH) {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent a response whose length is invalid to serial number request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  if (deviceResponse[0] != IR_IPT_SerialNumberResponse) {
    logBytes(LOG_ERR, DRIVER_LOG_PREFIX "The device has sent an unexpected response to serial number request", deviceResponse, size);
    closePort(&internalPort);
    return 0;
  } 
  if (deviceResponse[1] != IR_OPT_SERIALNUMBERRESPONSE_NOWINDOWLENGTH) {
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
