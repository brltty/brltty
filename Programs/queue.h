/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

#ifndef BRLTTY_INCLUDED_QUEUE
#define BRLTTY_INCLUDED_QUEUE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct QueueStruct Queue;
typedef struct ElementStruct Element;
typedef void (*ItemDeallocator) (void *item, void *data);
typedef int (*ItemComparator) (const void *item1, const void *item2, void *data);

extern Queue *newQueue (ItemDeallocator deallocate, ItemComparator compare);
extern void deallocateQueue (Queue *queue);
extern int getQueueSize (Queue *queue);
extern void *getQueueData (Queue *queue);
extern void *setQueueData (Queue *queue, void *data);

typedef int (*ItemProcessor) (void *item, void *data);
extern Element *processQueue (Queue *queue, ItemProcessor processItem, void *data);
extern void *findItem (Queue *queue, ItemProcessor testItem, void *data);

extern void deleteElement (Element *element);

extern Element *enqueueItem (Queue *queue, void *item);
extern void *dequeueItem (Queue *queue);
extern void deleteItem (Queue *queue, void *item);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_QUEUE */
