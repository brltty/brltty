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

#ifndef BRLTTY_INCLUDED_LOCK
#define BRLTTY_INCLUDED_LOCK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct LockDescriptorStruct LockDescriptor;

typedef enum {
  LOCK_Exclusive = 0X1,
  LOCK_NoWait   = 0X2
} LockOptions;

extern LockDescriptor *newLockDescriptor (void);
extern LockDescriptor *getLockDescriptor (LockDescriptor **lock);
extern void freeLockDescriptor (LockDescriptor *lock);

extern int obtainLock (LockDescriptor *lock, LockOptions options);
extern void releaseLock (LockDescriptor *lock);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_LOCK */
