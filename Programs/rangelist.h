/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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
 
#ifndef _RANGELIST_H
#define _RANGELIST_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Header file for the range list management module */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

typedef struct rangelist
{
 uint32_t x,y;
 struct rangelist *next;
} Trangelist;

/* Function : CreateRange */
/* Creates an item of a range list */
/* Specify the range, a pointer to previous and next item */
/* Returns the adress of the newly created range, NULL if not enough memory */
Trangelist *CreateRange(Trangelist *p, uint32_t x, uint32_t y, Trangelist *n);

/* Function : FreeRange */
/* Destroys an item of a range list */
/* If provided, the previous item will be linked to the next item */
void FreeRange(Trangelist *p, Trangelist *c);

/* Function : FreeRangeList */
/* Frees a whole list */
/* If you wan to destroy a whole list, call this function, rather than */
/* calling FreeRange on each element, since th latter cares about links */
/* and hence is slower */
void FreeRangeList(Trangelist **l);

/* Function : contains */
/* Determines if the range list l contains x */
/* If yes, returns the adress of the cell [a..b] such that a<=x<=b */
/* If no, returns NULL */
Trangelist *contains(Trangelist *l, uint32_t n);

/* Function : DisplayRangeList */
/* Prints a range list on stdout */
/* This is for debugging only */
void DisplayRangeList(Trangelist *l);

/* Function : AddRange */
/* Adds a range to a range list */
/* We have to keep a sorted list of [disjoints] ranges */
/* Return 0 if success, -1 if an error occurs */
int AddRange(uint32_t x0, uint32_t y0, Trangelist **l);

/* Function : RemoveRange */
/* Removes a range from a range list */
/* Returns 0 if success, -1 if failure */
int RemoveRange(uint32_t x0, uint32_t y0, Trangelist **l);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RANGELIST_H */
