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

/* Source file for range list management module */
/* For a description of what each function does, see rangelist.h */

#include "rangelist.h"
#include "misc.h"

/* Function : CreateRange */
Trangelist *CreateRange(Trangelist *p, uint32_t x, uint32_t y, Trangelist *n)
{
 Trangelist *c = (Trangelist *) malloc(sizeof(Trangelist));
 if (c==NULL) return NULL;
 c->x = x; c->y = y; c->next = n;
 if (p!=NULL) p->next = c;
 return c;
}

/* Function : FreeRange */
void FreeRange(Trangelist *p, Trangelist *c)
{
 if (c==NULL) return;
 if (p!=NULL) p->next = c->next;
 free(c);
}

/* Function : FreeRangeList */
void FreeRangeList(Trangelist **l)
{
 Trangelist *p1, *p2;
 if (l==NULL) return;
 p2 = *l;
 while (p2!=NULL)
 {
  p1 = p2;
  p2 = p1->next;
  free(p1);
 }
 *l = NULL;
}

/* Function : contains */
Trangelist *contains(Trangelist *l, uint32_t n)
{
 Trangelist *c = l;
 while (c!=NULL)
 {
  if (c->x>n) return NULL; /* the list is sorted! */
  if (n<=c->y) return c;
  c = c->next;
 }
 return NULL;
}

/* Function : DisplayRangeList */
void DisplayRangeList(Trangelist *l)
{
 if (l==NULL) {
  printf("emptyset");
 } else {
  Trangelist *c = l;
  while (1)
  {
   printf("[%u..%u]",c->x,c->y);
   if (c->next==NULL) break;
   printf(",");
   c = c->next;
  }
 }
 printf("\n");
}

/* Function : AddRange */
int AddRange(uint32_t x0, uint32_t y0, Trangelist **l)
{
 uint32_t x,y;
 Trangelist *p, *c, *tmp;
 x = MIN(x0,y0); y = MAX(x0,y0);
 if (*l == NULL)
 {
  if ((*l = CreateRange(NULL,x,y,NULL)) == NULL) return -1;
  return 0;
 }
 p = NULL; c = *l;
 while (1)
 {
  if (x<=c->x)
  {
   if (y<c->x-1)
   {
    tmp = CreateRange(p,x,y,c);
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
   if (x>c->y+1)
   {
    p = c; c = p->next;
    if (c==NULL)
    {
     c = CreateRange(p,x,y,NULL);
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
 /* here p the range that contains the left bound */
 /* We have now to look if p denotes a range that contains other ranges */
 while (1)
 {
  if (p->y<c->x-1) return 0;
  if (p->y<=c->y)
  {
   p->y = c->y;
   FreeRange(p,c);
   return 0;
  } else {
   FreeRange(p,c);
   c = p->next;
   if (c==NULL) return 0;
   continue;
  }
 }
}

int RemoveRange(uint32_t x0, uint32_t y0, Trangelist **l)
{
 int z;
 uint32_t x, y;
 Trangelist *p, *c, *tmp;

 if ((l==NULL) || (*l==NULL)) return 0;

 x = MIN(x0,y0); y = MAX(x0,y0);
 p = NULL; c = *l;

 while (1)
 {
  if (x<=c->x)
  {
   if (y<c->x-1)
   {
    p = c; c = p->next;
    if (c==NULL) return 0;
    continue;
   } else {
    if (y<c->y)
    {
     c->x = y+1;
     return 0;
    } else {
     z = (y == c->y);
     tmp = c; c = c->next;
     FreeRange(p,tmp);
     if (p==NULL) (*l) = c;
     if ((z) || (c==NULL)) return 0;
     continue;
    }
   }
  } else {
   if (x>c->y)
   {
    p = c; c = p->next;
    if (c==NULL) return 0; else continue;
   } else {
    if (x==c->x)
    {
     if (y<c->y)
     {
      c->x = y+1;
      return 0;
     } else {
      z = (y == c->y);
      tmp = c; c = c->next;
      FreeRange(p,tmp);
      if (p==NULL) (*l) = c;
      if ((z) || (c==NULL)) return 0;
      continue;
     }
    } else {
     if (y<c->y)
     {
      p = c;
      if ((c = CreateRange(p,c->x,c->y,p->next)) == NULL) return -1;
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

