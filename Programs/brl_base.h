/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_BRL_BASE
#define BRLTTY_INCLUDED_BRL_BASE

#include "brl_types.h"
#include "gio_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define KEY_TABLE_LIST_REFERENCE const KeyTableDefinition *const *
#define KEY_TABLE_LIST_SYMBOL CONCATENATE(brl_ktb_,DRIVER_CODE)
#define KEY_TABLE_LIST_DECLARATION const KeyTableDefinition *const KEY_TABLE_LIST_SYMBOL[]
#define LAST_KEY_TABLE_DEFINITION NULL
#define BEGIN_KEY_TABLE_LIST \
  extern KEY_TABLE_LIST_DECLARATION; \
  KEY_TABLE_LIST_DECLARATION = {
#define END_KEY_TABLE_LIST LAST_KEY_TABLE_DEFINITION};

#define BRL_SET_NAME(d,s) d ## _SET_ ## s
#define BRL_KEY_NAME(d,s,k) d ## _ ## s ## _ ## k

#define BRL_KEY_SET_ENTRY(d,s,n) KEY_SET_ENTRY(BRL_SET_NAME(d, s), n)
#define BRL_KEY_NAME_ENTRY(d,s,k,n) {.value={.set=BRL_SET_NAME(d, s), .key=BRL_KEY_NAME(d, s, k)}, .name=n}

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

typedef int BrailleSessionInitializer (BrailleDisplay *brl);

extern int connectBrailleResource (
  BrailleDisplay *brl,
  const char *identifier,
  const GioDescriptor *descriptor,
  BrailleSessionInitializer *initializeSession
);

typedef int BrailleSessionEnder (BrailleDisplay *brl);

extern void disconnectBrailleResource (
  BrailleDisplay *brl,
  BrailleSessionEnder *endSession
);

typedef enum {
  BRL_PVR_INVALID,
  BRL_PVR_INCLUDE,
  BRL_PVR_EXCLUDE
} BraillePacketVerifierResult;

typedef BraillePacketVerifierResult BraillePacketVerifier (
  BrailleDisplay *brl,
  const unsigned char *bytes, size_t size,
  size_t *length, void *data
);

extern size_t readBraillePacket (
  BrailleDisplay *brl,
  GioEndpoint *endpoint,
  void *packet, size_t size,
  BraillePacketVerifier *verifyPacket, void *data
);

extern int writeBraillePacket (
  BrailleDisplay *brl,
  GioEndpoint *endpoint,
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
  BrailleRequestWriter *writeRequest,
  BraillePacketReader *readPacket, void *responsePacket, size_t responseSize,
  BrailleResponseHandler *handleResponse
);

extern int cellsHaveChanged (
  unsigned char *cells, const unsigned char *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
);

extern int textHasChanged (
  wchar_t *text, const wchar_t *new, unsigned int count,
  unsigned int *from, unsigned int *to, int *force
);

extern int cursorHasChanged (int *cursor, int new, int *force);

typedef uint32_t KeyValueSet;
#define KEY_VALUE_BIT(number) (UINT32_C(1) << (number))

extern int enqueueKeyEvent (
  BrailleDisplay *brl,
  unsigned char set, unsigned char key, int press
);

extern int enqueueKeyEvents (
  BrailleDisplay *brl,
  KeyValueSet bits, unsigned char set, unsigned char key, int press
);

extern int enqueueKey (
  BrailleDisplay *brl,
  unsigned char set, unsigned char key
);

extern int enqueueKeys (
  BrailleDisplay *brl,
  KeyValueSet bits, unsigned char set, unsigned char key
);

extern int enqueueUpdatedKeys (
  BrailleDisplay *brl,
  KeyValueSet new, KeyValueSet *old, unsigned char set, unsigned char key
);

extern int enqueueXtScanCode (
  BrailleDisplay *brl,
  unsigned char code, unsigned char escape,
  unsigned char set00, unsigned char setE0, unsigned char setE1
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_BRL_BASE */
