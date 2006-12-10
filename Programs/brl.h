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

#ifndef BRLTTY_INCLUDED_BRL
#define BRLTTY_INCLUDED_BRL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "prologue.h"

#include "driver.h"
#include "brldefs.h"

/* status cell styles */
typedef enum {
  ST_None,
  ST_AlvaStyle,
  ST_TiemanStyle,
  ST_PB80Style,
  ST_Generic,
  ST_MDVStyle,
  ST_VoyagerStyle
} StatusCellsStyle;

/* Braille information structure. */
typedef struct {
  unsigned int x, y;			/* the dimensions of the display */
  unsigned int helpPage;			/* the page number within the help file */
  unsigned char *buffer;	/* the contents of the display */
  int cursor;		/* the position of the cursor within the display */
  unsigned isCoreBuffer:1;	/* the core allocated the buffer */
  unsigned resizeRequired:1;	/* the display size has changed */
  unsigned int writeDelay;
  void (*bufferResized) (int rows, int columns);
  const char *dataDirectory;
} BrailleDisplay;				/* used for writing to a braille display */

extern void initializeBrailleDisplay (BrailleDisplay *);
extern unsigned int drainBrailleOutput (BrailleDisplay *, int minimumDelay);
extern int allocateBrailleBuffer (BrailleDisplay *);

extern void writeBrailleBuffer (BrailleDisplay *);
extern void writeBrailleText (BrailleDisplay *, const char *, int);
extern void writeBrailleString (BrailleDisplay *, const char *);
extern void showBrailleString (BrailleDisplay *, const char *, unsigned int);

extern void clearStatusCells (BrailleDisplay *brl);
extern void setStatusText (BrailleDisplay *brl, const char *text);

extern int readBrailleCommand (BrailleDisplay *, BRL_DriverCommandContext);

typedef enum {
  BF_MINIMUM,
  BF_LOW,
  BF_MEDIUM,
  BF_HIGH,
  BF_MAXIMUM
} BrailleFirmness;

typedef enum {
  BS_MINIMUM,
  BS_LOW,
  BS_MEDIUM,
  BS_HIGH,
  BS_MAXIMUM
} BrailleSensitivity;

/* Routines provided by each braille driver.
 * These are loaded dynamically at run-time into this structure
 * with pointers to all the functions and variables.
 */
typedef struct {
  DRIVER_DEFINITION_DECLARATION;
  const char *const *parameters;
  const char *helpFile;
  int statusStyle;

  /* Routines provided by the braille driver library: */
  int (*open) (BrailleDisplay *, char **parameters, const char *);
  void (*close) (BrailleDisplay *);
  int (*readCommand) (BrailleDisplay *, BRL_DriverCommandContext);
  void (*writeWindow) (BrailleDisplay *);
  void (*writeStatus) (BrailleDisplay *brl, const unsigned char *);

  /* These require BRL_HAVE_VISUAL_DISPLAY. */
  void (*writeVisual) (BrailleDisplay *);

  /* These require BRL_HAVE_PACKET_IO. */
  ssize_t (*readPacket) (BrailleDisplay *, void *, size_t);
  ssize_t (*writePacket) (BrailleDisplay *, const void *, size_t);
  int (*reset) (BrailleDisplay *);
  
  /* These require BRL_HAVE_KEY_CODES. */
  int (*readKey) (BrailleDisplay *);
  int (*keyToCommand) (BrailleDisplay *, BRL_DriverCommandContext, int);

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
extern TranslationTable textTable;	 /* current text to braille translation table */
extern TranslationTable untextTable;     /* current braille to text translation table */
extern TranslationTable attributesTable; /* current attributes to braille translation table */
extern void *contractionTable; /* current braille contraction table */

extern void makeUntextTable (void);

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

extern void learnMode (BrailleDisplay *brl, int poll, int timeout);

extern void showDotPattern (unsigned char dots, unsigned char duration);
extern void setBrailleFirmness (BrailleDisplay *brl, int setting);
extern void setBrailleSensitivity (BrailleDisplay *brl, int setting);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL */
