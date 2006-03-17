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
 
#ifndef BRLTTY_INCLUDED_RANGELIST
#define BRLTTY_INCLUDED_RANGELIST

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Header file for the range list management module */

#include <inttypes.h>

typedef struct RangeList {
  uint32_t x, y;
  struct RangeList *next;
} RangeList;

/* Function : createRange */
/* Creates an item of a range list */
/* Specify the range, a pointer to previous and next item */
/* Returns the adress of the newly created range, NULL if not enough memory */
extern RangeList *createRange(RangeList *p, uint32_t x, uint32_t y, RangeList *n);

/* Function : freeRange */
/* Destroys an item of a range list */
/* If provided, the previous item will be linked to the next item */
extern void freeRange(RangeList *p, RangeList *c);

/* Function : freeRangeList */
/* Frees a whole list */
/* If you wan to destroy a whole list, call this function, rather than */
/* calling freeRange on each element, since th latter cares about links */
/* and hence is slower */
extern void freeRangeList(RangeList **l);

/* Function : inRangeList */
/* Determines if the range list l contains x */
/* If yes, returns the adress of the cell [a..b] such that a<=x<=b */
/* If no, returns NULL */
extern RangeList *inRangeList(RangeList *l, uint32_t n);

/* Function : displayRangeList */
/* Prints a range list on stdout */
/* This is for debugging only */
extern void displayRangeList(RangeList *l);

/* Function : addRange */
/* Adds a range to a range list */
/* We have to keep a sorted list of [disjoints] ranges */
/* Return 0 if success, -1 if an error occurs */
extern int addRange(uint32_t x0, uint32_t y0, RangeList **l);

/* Function : removeRange */
/* Removes a range from a range list */
/* Returns 0 if success, -1 if failure */
extern int removeRange(uint32_t x0, uint32_t y0, RangeList **l);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_RANGELIST */
