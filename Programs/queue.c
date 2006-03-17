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

#include "prologue.h"

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

  if (element->item) {
    if (queue->deallocate) queue->deallocate(element->item, queue->data);
    element->item = NULL;
  }

  element->queue = NULL;
  queue->size--;

  element->next = discardedElements;
  discardedElements = element;
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
    if (!(element = malloc(sizeof(*element)))) return NULL;
    element->previous = element->next = NULL;
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

static void
enqueueElement (Element *element) {
  Queue *queue = element->queue;
  if (queue->head) {
    linkAdditionalElement(queue->head, element);
  } else {
    linkFirstElement(element);
  }
}

Element *
enqueueItem (Queue *queue, void *item) {
  Element *element = newElement(queue, item);
  if (element) enqueueElement(element);
  return element;
}

void
requeueElement (Element *element) {
  unlinkElement(element);
  enqueueElement(element);
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
getElementQueue (Element *element) {
  return element->queue;
}

void *
getElementItem (Element *element) {
  return element->item;
}

Queue *
newQueue (ItemDeallocator deallocate, ItemComparator compare) {
  Queue *queue;
  if ((queue = malloc(sizeof(*queue)))) {
    queue->head = NULL;
    queue->size = 0;
    queue->data = NULL;
    queue->deallocate = deallocate;
    queue->compare = compare;
    return queue;
  }
  return NULL;
}

void
deleteElements (Queue *queue) {
  while (queue->head) deleteElement(queue->head);
}

void
deallocateQueue (Queue *queue) {
  deleteElements(queue);
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

int
deleteItem (Queue *queue, void *item) {
  Element *element = processQueue(queue, testItemAddress, item);
  if (!element) return 0;

  element->item = NULL;
  deleteElement(element);
  return 1;
}
