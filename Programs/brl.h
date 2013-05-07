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

#ifndef BRLTTY_INCLUDED_BRL
#define BRLTTY_INCLUDED_BRL

#include "prologue.h"

#include "driver.h"
#include "io_generic.h"
#include "brldefs.h"
#include "ktbdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct BrailleDataStruct BrailleData;

typedef struct BrailleDisplayStruct BrailleDisplay;
typedef int BrailleFirmnessSetter (BrailleDisplay *brl, BrailleFirmness setting);
typedef int BrailleSensitivitySetter (BrailleDisplay *brl, BrailleSensitivity setting);
typedef void BrailleKeyRotator (BrailleDisplay *brl, unsigned char *set, unsigned char *key);

struct BrailleDisplayStruct {
  unsigned int textColumns, textRows;
  unsigned int statusColumns, statusRows;

  const char *keyBindings;
  KEY_NAME_TABLES_REFERENCE keyNameTables;
  KeyTable *keyTable;

  unsigned char *buffer;
  int cursor;
  unsigned isCoreBuffer:1;
  unsigned resizeRequired:1;
  unsigned noDisplay:1;
  unsigned int writeDelay;
  void (*bufferResized) (unsigned int rows, unsigned int columns);
  unsigned touchEnabled:1;
  unsigned highlightWindow:1;
  BrailleData *data;

  BrailleFirmnessSetter *setFirmness;
  BrailleSensitivitySetter *setSensitivity;
  BrailleKeyRotator *rotateKey;
};

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

extern int enqueueCommand (int command);
extern int enqueueKeyEvent (unsigned char set, unsigned char key, int press);

extern int enqueueKey (unsigned char set, unsigned char key);
extern int enqueueKeys (uint32_t bits, unsigned char set, unsigned char key);
extern int enqueueUpdatedKeys (uint32_t new, uint32_t *old, unsigned char set, unsigned char key);

extern int enqueueXtScanCode (
  unsigned char code, unsigned char escape,
  unsigned char set00, unsigned char setE0, unsigned char setE1
);

extern int readBrailleCommand (BrailleDisplay *, KeyTableCommandContext);
extern KeyTableCommandContext getCurrentCommandContext (void);

extern int writeBraillePacket (
  BrailleDisplay *brl, GioEndpoint *endpoint,
  const void *packet, size_t size
);

typedef int BrailleRequestWriter (BrailleDisplay *brl);

typedef size_t BraillePacketReader (
  BrailleDisplay *brl,
  void *packet, size_t size
);

typedef enum {
  BRL_RSP_CONTINUE,
  BRL_RSP_DONE,
  BRL_RSP_FAIL,
  BRL_RSP_UNEXPECTED
} BrailleResponseResult;

typedef BrailleResponseResult BrailleResponseHandler (
  BrailleDisplay *brl,
  const void *packet, size_t size
);

extern int probeBrailleDisplay (
  BrailleDisplay *brl, unsigned int retryLimit,
  GioEndpoint *endpoint, int inputTimeout,
  BrailleRequestWriter writeRequest,
  BraillePacketReader readPacket, void *responsePacket, size_t responseSize,
  BrailleResponseHandler *handleResponse
);

extern int setBrailleFirmness (BrailleDisplay *brl, BrailleFirmness setting);
extern int setBrailleSensitivity (BrailleDisplay *brl, BrailleSensitivity setting);

/* Routines provided by each braille driver.
 * These are loaded dynamically at run-time into this structure
 * with pointers to all the functions and variables.
 */
typedef struct {
  DRIVER_DEFINITION_DECLARATION;

  const char *const *parameters;
  const unsigned char *statusFields;

  int (*construct) (BrailleDisplay *brl, char **parameters, const char *device);
  void (*destruct) (BrailleDisplay *brl);

  int (*readCommand) (BrailleDisplay *brl, KeyTableCommandContext context);
  int (*writeWindow) (BrailleDisplay *brl, const wchar_t *characters);
  int (*writeStatus) (BrailleDisplay *brl, const unsigned char *cells);

  ssize_t (*readPacket) (BrailleDisplay *brl, void *buffer, size_t size);
  ssize_t (*writePacket) (BrailleDisplay *brl, const void *packet, size_t size);
  int (*reset) (BrailleDisplay *brl);
  
  int (*readKey) (BrailleDisplay *brl);
  int (*keyToCommand) (BrailleDisplay *brl, KeyTableCommandContext context, int key);
} BrailleDriver;

extern int haveBrailleDriver (const char *code);
extern const char *getDefaultBrailleDriver (void);
extern const BrailleDriver *loadBrailleDriver (const char *code, void **driverObject, const char *driverDirectory);
extern void identifyBrailleDriver (const BrailleDriver *driver, int full);
extern void identifyBrailleDrivers (int full);
extern const BrailleDriver *braille;
extern const BrailleDriver noBraille;

extern int cellsHaveChanged (
  unsigned char *cells, const unsigned char *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
);

extern int textHasChanged (
  wchar_t *text, const wchar_t *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
);

extern int cursorHasChanged (int *cursor, int new, int *force);

#define TRANSLATION_TABLE_SIZE 0X100
typedef unsigned char TranslationTable[TRANSLATION_TABLE_SIZE];

#define DOTS_TABLE_SIZE 8
typedef unsigned char DotsTable[DOTS_TABLE_SIZE];
extern const DotsTable dotsTable_ISO11548_1;

extern void makeTranslationTable (const DotsTable dots, TranslationTable table);
extern void reverseTranslationTable (const TranslationTable from, TranslationTable to);

extern void setOutputTable (const TranslationTable table);
extern void makeOutputTable (const DotsTable dots);
extern void *translateOutputCells (unsigned char *target, const unsigned char *source, size_t count);
extern unsigned char translateOutputCell (unsigned char cell);

extern void makeInputTable (void);
extern void *translateInputCells (unsigned char *target, const unsigned char *source, size_t count);
extern unsigned char translateInputCell (unsigned char cell);

extern void applyBrailleOrientation (unsigned char *cells, size_t count);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL */
