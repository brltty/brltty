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

/* Source file for range list management module */
/* For a description of what each function does, see rangelist.h */

#include "rangelist.h"
#include "misc.h"

/* Function : createRange */
rangeList *createRange(rangeList *p, uint32_t x, uint32_t y, rangeList *n)
{
  rangeList *c = malloc(sizeof(rangeList));
  if (c==NULL) return NULL;
  c->x = x; c->y = y; c->next = n;
  if (p!=NULL) p->next = c;
  return c;
}

/* Function : freeRange */
void freeRange(rangeList *p, rangeList *c)
{
  if (c==NULL) return;
  if (p!=NULL) p->next = c->next;
  free(c);
}

/* Function : freeRangeList */
void freeRangeList(rangeList **l)
{
  rangeList *p1, *p2;
  if (l==NULL) return;
  p2 = *l;
  while (p2!=NULL) {
    p1 = p2;
    p2 = p1->next;
    free(p1);
  }
  *l = NULL;
}

/* Function : contains */
rangeList *contains(rangeList *l, uint32_t n)
{
  rangeList *c = l;
  while (c!=NULL) {
    if (c->x>n) return NULL; /* the list is sorted! */
    if (n<=c->y) return c;
    c = c->next;
  }
  return NULL;
}

/* Function : DisplayRangeList */
void DisplayRangeList(rangeList *l)
{
  if (l==NULL) printf("emptyset");
  else {
    rangeList *c = l;
    while (1) {
      printf("[%lu..%lu]",(unsigned long)c->x,(unsigned long)c->y);
      if (c->next==NULL) break;
      printf(",");
      c = c->next;
    }
  }
  printf("\n");
}

/* Function : addRange */
int addRange(uint32_t x0, uint32_t y0, rangeList **l)
{
  uint32_t x,y;
  rangeList *p, *c, *tmp;
  x = MIN(x0,y0); y = MAX(x0,y0);
  if (*l == NULL) {
    if ((*l = createRange(NULL,x,y,NULL)) == NULL) return -1;
    return 0;
  }
  p = NULL; c = *l;
  while (1) {
    if (x<=c->x) {
      if (y<c->x-1) {
        tmp = createRange(p,x,y,c);
        if (tmp==NULL) return 0;
        if (p==NULL) *l = tmp;
        return 0;
      } else {
        c->x = x;
        if (c->y>=y) return 0;
        c->y = y;
        p = c; c = p->next;
        if (c==NULL) return 0;
        break;
      }
    } else {
      if (x>c->y+1) {
        p = c; c = p->next;
        if (c==NULL) {
          c = createRange(p,x,y,NULL);
          if (c==NULL) return -1; else return 0;
        } else continue;
      } else {
        c->y = y;
        p = c; c = p->next;
        if (c==NULL) return 0;
        break;
      }
    }
  }
  /* here p is the range that contains the left bound */
  /* We have now to look if p denotes a range that contains other ranges */
  while (1) {
    if (p->y<c->x-1) return 0;
    if (p->y<=c->y) {
      p->y = c->y;
      freeRange(p,c);
      return 0;
    } else {
      freeRange(p,c);
      c = p->next;
      if (c==NULL) return 0;
      continue;
    }
  }
}

int removeRange(uint32_t x0, uint32_t y0, rangeList **l)
{
  int z;
  uint32_t x, y;
  rangeList *p, *c, *tmp;

  if ((l==NULL) || (*l==NULL)) return 0;

  x = MIN(x0,y0); y = MAX(x0,y0);
  p = NULL; c = *l;

  while (1) {
    if (x<=c->x) {
      if (y<c->x-1) {
        p = c; c = p->next;
        if (c==NULL) return 0;
        continue;
      } else {
        if (y<c->y) {
          c->x = y+1;
          return 0;
        } else {
          z = (y == c->y);
          tmp = c; c = c->next;
          freeRange(p,tmp);
          if (p==NULL) (*l) = c;
          if ((z) || (c==NULL)) return 0;
          continue;
        }
      }
    } else {
      if (x>c->y) {
        p = c; c = p->next;
        if (c==NULL) return 0; else continue;
      } else {
        if (x==c->x) {
          if (y<c->y) {
            c->x = y+1;
            return 0;
          } else {
            z = (y == c->y);
            tmp = c; c = c->next;
            freeRange(p,tmp);
            if (p==NULL) (*l) = c;
            if ((z) || (c==NULL)) return 0;
            continue;
          }
        } else {
          if (y<c->y) {
            p = c;
            if ((c = createRange(p,c->x,c->y,p->next)) == NULL) return -1;
            p->y = x-1;
            c->x = y+1;
            return 0;
          } else {
            z = (y == c->y);
            c->y = x-1;
            if (z) return 0;
            p = c; c = p->next;
            if (c==NULL) return 0; else continue;
          }
        }
      }
    }
  }
}
