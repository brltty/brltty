/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2009 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_BRL
#define BRLTTY_INCLUDED_BRL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#include "driver.h"
#include "brldefs.h"
#include "keydefs.h"

typedef enum {
  sfEnd,
  sfWindowCoordinates,
  sfWindowColumn,
  sfWindowRow,
  sfCursorCoordinates,
  sfCursorColumn,
  sfCursorRow,
  sfCursorAndWindowColumn,
  sfCursorAndWindowRow,
  sfScreenNumber,
  sfStateDots,
  sfStateLetter,
  sfTime,
  sfAlphabeticWindowCoordinates,
  sfAlphabeticCursorCoordinates,
  sfGeneric
} StatusField;

typedef struct BrailleDataStruct BrailleData;

typedef struct {
  unsigned int textColumns, textRows;
  unsigned int statusColumns, statusRows;
  const char *keyBindings;
  unsigned char *buffer;
  int cursor;
  unsigned isCoreBuffer:1;
  unsigned resizeRequired:1;
  unsigned int writeDelay;
  void (*bufferResized) (int infoLevel, int rows, int columns);
  const char *dataDirectory;
  unsigned touchEnabled:1;
  unsigned highlightWindow:1;
  BrailleData *data;
} BrailleDisplay;

extern void initializeBrailleDisplay (BrailleDisplay *brl);
extern unsigned int drainBrailleOutput (BrailleDisplay *brl, int minimumDelay);
extern int ensureBrailleBuffer (BrailleDisplay *brl, int infoLevel);

extern void fillTextRegion (
  wchar_t *text, unsigned char *dots,
  unsigned int start, unsigned int count,
  unsigned int columns, unsigned int rows,
  const wchar_t *characters, size_t length
);

extern void fillDotsRegion (
  wchar_t *text, unsigned char *dots,
  unsigned int start, unsigned int count,
  unsigned int columns, unsigned int rows,
  const unsigned char *cells, size_t length
);

extern int clearStatusCells (BrailleDisplay *brl);
extern int setStatusText (BrailleDisplay *brl, const char *text);

extern int readBrailleCommand (BrailleDisplay *, BRL_DriverCommandContext);

typedef enum {
  BRL_FIRMNESS_MINIMUM,
  BRL_FIRMNESS_LOW,
  BRL_FIRMNESS_MEDIUM,
  BRL_FIRMNESS_HIGH,
  BRL_FIRMNESS_MAXIMUM
} BrailleFirmness;
extern void setBrailleFirmness (BrailleDisplay *brl, BrailleFirmness setting);

typedef enum {
  BRL_SENSITIVITY_MINIMUM,
  BRL_SENSITIVITY_LOW,
  BRL_SENSITIVITY_MEDIUM,
  BRL_SENSITIVITY_HIGH,
  BRL_SENSITIVITY_MAXIMUM
} BrailleSensitivity;
extern void setBrailleSensitivity (BrailleDisplay *brl, BrailleSensitivity setting);

/* Routines provided by each braille driver.
 * These are loaded dynamically at run-time into this structure
 * with pointers to all the functions and variables.
 */
typedef struct {
  DRIVER_DEFINITION_DECLARATION;
  const char *const *parameters;
  const unsigned char *statusFields;

  /* Routines provided by the braille driver library: */
  int (*construct) (BrailleDisplay *brl, char **parameters, const char *device);
  void (*destruct) (BrailleDisplay *brl);
  int (*readCommand) (BrailleDisplay *brl, BRL_DriverCommandContext context);
  int (*writeWindow) (BrailleDisplay *brl, const wchar_t *characters);

  /* These require BRL_HAVE_STATUS_CELLS. */
  int (*writeStatus) (BrailleDisplay *brl, const unsigned char *cells);

  /* These require BRL_HAVE_PACKET_IO. */
  ssize_t (*readPacket) (BrailleDisplay *brl, void *buffer, size_t size);
  ssize_t (*writePacket) (BrailleDisplay *brl, const void *packet, size_t size);
  int (*reset) (BrailleDisplay *brl);
  
  /* These require BRL_HAVE_KEY_CODES. */
  int (*readKey) (BrailleDisplay *brl);
  int (*keyToCommand) (BrailleDisplay *brl, BRL_DriverCommandContext context, int key);

  /* These require BRL_HAVE_FIRMNESS. */
  void (*firmness) (BrailleDisplay *brl, BrailleFirmness setting);

  /* These require BRL_HAVE_SENSITIVITY. */
  void (*sensitivity) (BrailleDisplay *brl, BrailleSensitivity setting);
} BrailleDriver;

extern int haveBrailleDriver (const char *code);
extern const char *getDefaultBrailleDriver (void);
extern const BrailleDriver *loadBrailleDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void identifyBrailleDriver (const BrailleDriver *driver, int full);
extern void identifyBrailleDrivers (int full);
extern const BrailleDriver *braille;
extern const BrailleDriver noBraille;

#define TRANSLATION_TABLE_SIZE 0X100
typedef unsigned char TranslationTable[TRANSLATION_TABLE_SIZE];
extern void reverseTranslationTable (TranslationTable from, TranslationTable to);

#define DOTS_TABLE_SIZE 8
typedef unsigned char DotsTable[DOTS_TABLE_SIZE];
extern void makeOutputTable (const DotsTable dots, TranslationTable table);

/* Formatting of status cells. */
extern unsigned char lowerDigit (unsigned char upper);
extern const unsigned char landscapeDigits[11];
extern int landscapeNumber (int x);
extern int landscapeFlag (int number, int on);
extern const unsigned char seascapeDigits[11];
extern int seascapeNumber (int x);
extern int seascapeFlag (int number, int on);
extern const unsigned char portraitDigits[11];
extern int portraitNumber (int x);
extern int portraitFlag (int number, int on);

extern int learnMode (BrailleDisplay *brl, int poll, int timeout);

extern int enqueueKeyEvent (KeyCode code, int press);
extern int dequeueKeyEvent (KeyCode *code, int *press);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL */
