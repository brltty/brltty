/*
 * BRLTTY - A background process providing access to the console screen (when in
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>

#include "misc.h"
#include "queue.h"

struct QueueStruct {
  Element *head;
  unsigned int size;
  void *data;
  ItemDeallocator deallocate;
  ItemComparator compare;
};

struct ElementStruct {
  Element *next;
  Element *previous;
  Queue *queue;
  void *item;
};

static Element *discardedElements = NULL;

static void
discardElement (Element *element) {
  Queue *queue = element->queue;

  element->queue = NULL;
  queue->size--;

  element->next = discardedElements;
  discardedElements = element;

  if (element->item) {
    if (queue->deallocate) queue->deallocate(element->item, queue->data);
    element->item = NULL;
  }
}

static Element *
retrieveElement (void) {
  if (discardedElements) {
    Element *element = discardedElements;
    discardedElements = element->next;
    element->next = NULL;
    return element;
  }

  return NULL;
}

static Element *
newElement (Queue *queue, void *item) {
  Element *element;

  if (!(element = retrieveElement())) {
    if ((element = malloc(sizeof(*element)))) {
      element->previous = element->next = NULL;
    } else {
      return NULL;
    }
  }

  element->queue = queue;
  queue->size++;

  element->item = item;
  return element;
}

static void
linkFirstElement (Element *element) {
  element->queue->head = element->previous = element->next = element;
}

static void
linkAdditionalElement (Element *reference, Element *element) {
  element->next = reference;
  element->previous = reference->previous;
  element->next->previous = element;
  element->previous->next = element;
}

static void
unlinkElement (Element *element) {
  Queue *queue = element->queue;
  if (element == element->next) {
    queue->head = NULL;
  } else {
    if (element == queue->head) queue->head = element->next;
    element->next->previous = element->previous;
    element->previous->next = element->next;
  }
  element->previous = element->next = NULL;
}

void
deleteElement (Element *element) {
  unlinkElement(element);
  discardElement(element);
}

Element *
enqueueItem (Queue *queue, void *item) {
  Element *element = newElement(queue, item);
  if (element) {
    if (queue->head) {
      linkAdditionalElement(queue->head, element);
    } else {
      linkFirstElement(element);
    }
  }
  return element;
}

void *
dequeueItem (Queue *queue) {
  void *item;
  Element *element;

  if (!(element = queue->head)) return NULL;
  item = element->item;
  element->item = NULL;

  deleteElement(element);
  return item;
}

Queue *
newQueue (ItemDeallocator deallocate, ItemComparator compare) {
  Queue *queue;
  if ((queue = malloc(sizeof(*queue)))) {
    queue->head = NULL;
    queue->size = 0;
    queue->deallocate = deallocate;
    queue->compare = compare;
    return queue;
  }
  return NULL;
}

void
deallocateQueue (Queue *queue) {
  while (queue->head) deleteElement(queue->head);
  free(queue);
}

int
getQueueSize (Queue *queue) {
  return queue->size;
}

void *
getQueueData (Queue *queue) {
  return queue->data;
}

void *
setQueueData (Queue *queue, void *data) {
  void *previous = queue->data;
  queue->data = data;
  return previous;
}

Element *
processQueue (Queue *queue, ItemProcessor processItem, void *data) {
  Element *element = queue->head;
  while (element) {
    Element *next = element->next;
    if (processItem(element->item, data)) return element;
    element = (next != queue->head)? next: NULL;
  }
  return NULL;
}

void *
findItem (Queue *queue, ItemProcessor testItem, void *data) {
  Element *element = processQueue(queue, testItem, data);
  if (element) return element->item;
  return NULL;
}

static int
testItemAddress (void *item, void *data) {
  return item == data;
}

void
deleteItem (Queue *queue, void *item) {
  Element *element = processQueue(queue, testItemAddress, item);
  if (element) {
    element->item = NULL;
    deleteElement(element);
  } else {
    LogPrint(LOG_WARNING, "Item not found: %p", item);
  }
}
