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

#ifndef _QUEUE_H
#define _QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct QueueStruct Queue;
typedef struct ElementStruct Element;
typedef void (*ItemDeallocator) (void *item);
typedef int (*ItemComparator) (const void *item1, const void *item2);

extern Queue *newQueue (ItemDeallocator deallocate, ItemComparator compare);
extern void deallocateQueue (Queue *queue);
extern void scanQueue (Queue *queue);

extern void deleteElement (Element *element);

extern Element *enqueueItem (Queue *queue, void *item);
extern void *dequeueItem (Queue *queue);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _QUEUE_H */
