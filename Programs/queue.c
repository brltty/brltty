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
    if (queue->deallocate) queue->deallocate(element->item);
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
linkElementBefore (Element *reference, Element *element) {
  element->next = reference;
  element->previous = reference->previous;
  element->next->previous = element;
  element->previous->next = element;
}

static void
linkElementAfter (Element *reference, Element *element) {
  linkElementBefore(reference->next, element);
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
  if (queue->head) {
    linkElementBefore(queue->head, element);
  } else {
    linkFirstElement(element);
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
    free(queue);
  }
  return NULL;
}

void
deallocateQueue (Queue *queue) {
  while (queue->head) deleteElement(queue->head);
  while (discardedElements) free(retrieveElement());
  free(queue);
}

void
scanQueue (Queue *queue) {
  Element *element = queue->head;
  while (element) {
    Element *next = element->next;
    element = (next != queue->head)? next: NULL;
  }
}
