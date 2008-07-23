/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

#ifndef BRLTTY_INCLUDED_DATAAREA
#define BRLTTY_INCLUDED_DATAAREA

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct DataAreaStruct DataArea;
extern DataArea *newDataArea (void);
extern void destroyDataArea (DataArea *area);
extern void clearDataArea (DataArea *area);

typedef unsigned long int DataOffset;
extern int allocateDataItem (DataArea *area, DataOffset *offset, int count, int alignment);
extern void *getDataItem (DataArea *area, DataOffset offset);
extern int saveDataItem (DataArea *area, DataOffset *offset, const void *item, int count, int alignment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_DATAAREA */
