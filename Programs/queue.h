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

extern Element *getQueueHead (const Queue *queue);
extern int getQueueSize (const Queue *queue);
extern void *getQueueData (const Queue *queue);
extern void *setQueueData (Queue *queue, void *data);

extern Element *enqueueItem (Queue *queue, void *item);
extern void *dequeueItem (Queue *queue);
extern int deleteItem (Queue *queue, const void *item);

extern Queue *getElementQueue (const Element *element);
extern void *getElementItem (const Element *element);

extern void deleteElements (Queue *queue);
extern void deleteElement (Element *element);
extern void requeueElement (Element *element);

typedef int (*ItemTester) (const void *item, const void *data);
extern Element *findElement (const Queue *queue, ItemTester testItem, const void *data);
extern void *findItem (const Queue *queue, ItemTester testItem, const void *data);

typedef int (*ItemProcessor) (void *item, void *data);
extern Element *processQueue (Queue *queue, ItemProcessor processItem, void *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_QUEUE */
