/*
 * BRLTTY - A background process providing access to AT-SPI terminals
 *          for a blind person using a refreshable braille display.
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <wchar.h>
#include <pthread.h>
#include <limits.h>
#include <langinfo.h>
#include <semaphore.h>
#include <locale.h>

#include <cspi/spi.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>

#include "Programs/misc.h"
#include "Programs/brldefs.h"

#include "Programs/scr_driver.h"

static AccessibleText *curTerm;
static Accessible *curFocus;

static long curNumRows, curNumCols;
static wchar_t **curRows;
static long *curRowLengths;
static long curPosX,curPosY;
static pthread_mutex_t updateMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t SPI_main_thread;

/* having our own implementation is much more independant on locales */

typedef struct {
  int remaining;
  wint_t current;
} my_mbstate_t;

static my_mbstate_t internal;

static size_t my_mbrtowc(wchar_t *pwc, const char *s, size_t n, my_mbstate_t *ps) {
  const unsigned char *c = (const unsigned char *) s;
  int read = 0;
  if (!c) {
    if (ps->remaining) {
      errno = EILSEQ;
      return (size_t)(-1);
    }
    return 0;
  }

  if (n && !ps->remaining) {
    /* initial state */
    if (!(*c&0x80)) {
      /* most frequent case: ascii */
      if (pwc)
	*pwc = *c;
      if (!*c)
	return 0;
      return 1;
    } else if (!(*c&0x40)) {
      /* utf-8 char continuation, shouldn't happen with remaining == 0 ! */
      goto error;
    } else {
      /* new utf-8 char, get remaining chars */
      read = 1;
      if (!(*c&0x20)) {
	ps->remaining = 1;
	ps->current = *c&((1<<5)-1);
      } else if (!(*c&0x10)) {
	ps->remaining = 2;
	ps->current = *c&((1<<4)-1);
      } else if (!(*c&0x08)) {
	ps->remaining = 3;
	ps->current = *c&((1<<3)-1);
      } else if (!(*c&0x04)) {
	ps->remaining = 4;
	ps->current = *c&((1<<2)-1);
      } else if (!(*c&0x02)) {
	ps->remaining = 5;
	ps->current = *c&((1<<1)-1);
      } else
	/* 0xff and 0xfe are not allowed */
	goto error;
      c++;
    }
  }
  /* looking for continuation chars */
  while (n-read) {
    if ((*c&0xc0) != 0X80)
      /* not continuation char, error ! */
      goto error;
    /* utf-8 char continuation */
    ps->current = (ps->current<<6) | (*c&((1<<6)-1));
    read++;
    if (!(--ps->remaining)) {
      if (pwc)
	*pwc = ps->current;
      if (!ps->current)
	/* shouldn't coded this way, but well... */
	return 0;
      return read;
    }
    c++;
  }
  return (size_t)(-2);
error:
  errno = EILSEQ;
  return (size_t)(-1);
}

static size_t my_mbsrtowcs(wchar_t *dest, const char **src, size_t len, my_mbstate_t *ps) {
  int put = 0;
  size_t skip;
  wchar_t buf,*bufp;

  if (!ps) ps = &internal;
  if (dest)
    bufp = dest;
  else
    bufp = &buf;

  while (len-put || !dest) {
    skip = my_mbrtowc(bufp, *src, 6, ps);
    switch (skip) {
      case (size_t)(-2): errno = EILSEQ; /* shouldn't happen ! */
      case (size_t)(-1): return (size_t)(-1);
      case 0: *src = NULL; return put;
    }
    *src += skip;
    if (dest) bufp++;
    put++;
  }
  return put;
}

static size_t my_mbrlen(const char *s, size_t n, my_mbstate_t *ps) {
  return my_mbrtowc(NULL, s, n, ps?ps:&internal);
}

static size_t my_mbslen(const char *s, size_t n) {
  my_mbstate_t ps;
  size_t ret=0;
  size_t eaten;
  memset(&ps,0,sizeof(ps));
  while(n) {
    if ((eaten = my_mbrlen(s,n,&ps))<0)
      return eaten;
    if (!(eaten)) return ret;
    s+=eaten;
    n-=eaten;
    ret++;
  }
  return ret;
}

static void addRows(long pos, long num) {
  curNumRows += num;
  curRows = realloc(curRows,curNumRows*sizeof(*curRows));
  curRowLengths = realloc(curRowLengths,curNumRows*sizeof(*curRowLengths));
  memmove(curRows      +pos+num,curRows      +pos,(curNumRows-(pos+num))*sizeof(*curRows));
  memmove(curRowLengths+pos+num,curRowLengths+pos,(curNumRows-(pos+num))*sizeof(*curRowLengths));
}

static void delRows(long pos, long num) {
  long y;
  for (y=pos;y<pos+num;y++)
    free(curRows[y]);
  memmove(curRows      +pos,curRows      +pos+num,(curNumRows-(pos+num))*sizeof(*curRows));
  memmove(curRowLengths+pos,curRowLengths+pos+num,(curNumRows-(pos+num))*sizeof(*curRowLengths));
  curNumRows -= num;
  curRows = realloc(curRows,curNumRows*sizeof(*curRows));
  curRowLengths = realloc(curRowLengths,curNumRows*sizeof(*curRowLengths));
}

static int
prepare_AtSpiScreen (char **parameters) {
  return 1;
}

static void findPosition(long position, long *px, long *py) {
  long offset=0, newoffset, x, y;
  /* XXX: I don't know what they do with necessary combining accents */
  for (y=0; y<curNumRows; y++) {
    if ((newoffset = offset + curRowLengths[y]) > position)
      break;
    offset = newoffset;
  }
  if (y==curNumRows) {
    if (!curNumRows) {
      y = 0;
      x = 0;
    } else {
      /* this _can_ happen, when deleting while caret is at the end of the
       * terminal: caret position is only updated afterwards... In the
       * meanwhile, keep caret at the end of last line. */
      y = curNumRows-1;
      x = curRowLengths[y];
    }
  } else
    x = position-offset;
  *px = x;
  *py = y;
}

static void caretPosition(long caret) {
  findPosition(caret,&curPosX,&curPosY);
}

static void finiTerm(void) {
  LogPrint(LOG_DEBUG,"end of term %p",curTerm);
  Accessible_unref(curTerm);
  curTerm = NULL;
  Accessible_unref(curFocus);
  curFocus = NULL;
  curPosX = curPosY = 0;
}

static void restartTerm(Accessible *newTerm) {
  char *c,*d;
  const char *e;
  long i,len;
  char *text;

  if (curFocus)
    finiTerm();
  Accessible_ref(curFocus = newTerm);
  curTerm = Accessible_getText(newTerm);
  LogPrint(LOG_DEBUG,"new term %p",curTerm);
  text = AccessibleText_getText(curTerm,0,LONG_MAX);
  curNumRows = 0;
  if (curRows) {
    for (i=0;i<curNumRows;i++)
      free(curRows[i]);
    free(curRows);
  }
  free(curRowLengths);
  c = text;
  while (*c) {
    curNumRows++;
    if (!(c = strchr(c,'\n')))
      break;
    c++;
  }
  LogPrint(LOG_DEBUG,"%ld rows",curNumRows);
  curRows = malloc(curNumRows * sizeof(*curRows));
  curRowLengths = malloc(curNumRows * sizeof(*curRowLengths));
  i = 0;
  curNumCols = 0;
  for (c = text; *c; c = d+1) {
    d = strchr(c,'\n');
    if (d)
      *d = 0;
    e = c;
    curRowLengths[i] = (len = my_mbsrtowcs(NULL,&e,0,NULL)) + (d != NULL);
    if (len > curNumCols)
      curNumCols = len;
    else if (len < 0) {
      if (len==-2)
	LogPrint(LOG_ERR,"unterminated sequence %s",c);
      else if (len==-1)
	LogError("mbrlen");
      curRowLengths[i] = (len = -1) + (d != NULL);
    }
    curRows[i] = malloc((len + (d!=NULL)) * sizeof(*curRows[i]));
    e = c;
    my_mbsrtowcs(curRows[i],&e,len,NULL);
    if (d)
      curRows[i][len]='\n';
    else
      break;
    i++;
  }
  LogPrint(LOG_DEBUG,"%ld cols",curNumCols);
  SPI_freeString(text);
  caretPosition(AccessibleText_getCaretOffset(curTerm));
}

static struct evList {
  struct evList *next;
  const AccessibleEvent *ev;
} *evs;

static void evListenerCB(const AccessibleEvent *event, void *user_data) {
  static int running = 0;
  struct evList *ev = malloc(sizeof(*ev));
  AccessibleEvent_ref(event);
  ev->next = evs;
  ev->ev = event;
  evs = ev;
  int state_changed_focused;

  /* this is not atomic but we can only be recursively called within calls
   * to the lib */
  if (running)
    return;
  else
    running = 1;

  while (evs) {
    pthread_mutex_lock(&updateMutex);
    /* pickup a list of events to handle */
    ev = evs;
    evs = NULL;
    for (; ev; AccessibleEvent_unref(ev->ev), ev = ev->next) {
      event = ev->ev;
      state_changed_focused = !strcmp(event->type,"object:state-changed:focused");
      if (state_changed_focused && !event->detail1) {
	if (event->source == curFocus)
	  finiTerm();
      } else if (!strcmp(event->type,"focus:") || (state_changed_focused && event->detail1)) {
	if (Accessible_getRole(event->source) != SPI_ROLE_TERMINAL) {
	  if (curFocus)
	    finiTerm();
	} else if (event->source != curFocus)
	  restartTerm(event->source);
      } else if (!strcmp(event->type,"object:text-caret-moved")) {
	if (event->source != curFocus) continue;
	caretPosition(event->detail1);
      } else if (!strcmp(event->type,"object:text-changed:delete")) {
	long x,y,toDelete = event->detail2;
	long length, toCopy;
	long downTo; /* line that will provide what will follow x */
	if (event->source != curFocus) continue;
	findPosition(event->detail1,&x,&y);
	downTo = y;
	length = curRowLengths[downTo];
	while (x+toDelete >= length) {
	  downTo++;
	  if (downTo <= curNumRows - 1)
	    length += curRowLengths[downTo];
	  else {
	    /* imaginary extra line doesn't provide more length, and shouldn't need to ! */
	    if (x+toDelete > length) {
	      LogPrint(LOG_ERR,"deleting past end of text !");
	      /* discarding */
	      toDelete = length - x;
	    }
	    break; /* deleting up to end */
	  }
	}
	if (length-toDelete>0) {
	  /* still something on line y */
	  if (y!=downTo) {
	    curRowLengths[y] = length-toDelete;
	    curRows[y]=realloc(curRows[y],curRowLengths[y]*sizeof(*curRows[y]));
	  }
	  if ((toCopy = length-toDelete-x))
	    memmove(curRows[y]+x,curRows[downTo]+curRowLengths[downTo]-toCopy,toCopy*sizeof(curRows[downTo]));
	  if (y==downTo) {
	    curRowLengths[y] = length-toDelete;
	    curRows[y]=realloc(curRows[y],curRowLengths[y]*sizeof(*curRows[y]));
	  }
	} else {
	  /* kills this line as well ! */
	  y--;
	}
	if (downTo==curNumRows)
	  /* imaginary extra line needs not be deleted */
	  downTo--;
	delRows(y+1,downTo-y);
	caretPosition(AccessibleText_getCaretOffset(curTerm));
      } else if (!strcmp(event->type,"object:text-changed:insert")) {
	long len=event->detail2,semilen,x,y;
	char *added;
	const char *adding,*c;
	if (event->source != curFocus) continue;
	findPosition(event->detail1,&x,&y);
	adding = c = added = AccessibleTextChangedEvent_getChangeString(event);
	if (x && (c = strchr(adding,'\n'))) {
	  /* splitting line */
	  addRows(y,1);
	  semilen=my_mbslen(adding,c+1-adding);
	  curRowLengths[y]=x+semilen;
	  if (x+semilen-1>curNumCols)
	    curNumCols=x+semilen-1;

	  /* copy beginning */
	  curRows[y]=malloc(curRowLengths[y]*sizeof(*curRows[y]));
	  memcpy(curRows[y],curRows[y+1],x*sizeof(*curRows[y]));
	  /* add */
	  my_mbsrtowcs(curRows[y]+x,&adding,semilen,NULL);
	  len-=semilen;
	  adding=c+1;
	  /* shift end */
	  curRowLengths[y+1]-=x;
	  memmove(curRows[y+1],curRows[y+1]+x,curRowLengths[y+1]*sizeof(*curRows[y+1]));
	  x=0;
	  y++;
	}
	while ((c = strchr(adding,'\n'))) {
	  /* adding lines */
	  addRows(y,1);
	  semilen=my_mbslen(adding,c+1-adding);
	  curRowLengths[y]=semilen;
	  if (semilen-1>curNumCols)
	    curNumCols=semilen-1;
	  curRows[y]=malloc(semilen*sizeof(*curRows[y]));
	  my_mbsrtowcs(curRows[y],&adding,semilen,NULL);
	  len-=semilen;
	  adding=c+1;
	  y++;
	}
	if (len) {
	  /* still length to add on the line following it */
	  if (y==curNumRows) {
	    /* It won't insert ending \n yet */
	    addRows(y,1);
	    curRows[y]=NULL;
	    curRowLengths[y]=0;
	  }
	  curRowLengths[y] += len;
	  curRows[y]=realloc(curRows[y],curRowLengths[y]*sizeof(*curRows[y]));
	  memmove(curRows[y]+x+len,curRows[y]+x,(curRowLengths[y]-(x+len))*sizeof(*curRows[y]));
	  my_mbsrtowcs(curRows[y]+x,&adding,len,NULL);
	  if (curRowLengths[y]-(curRows[y][curRowLengths[y]-1]=='\n')>curNumCols)
	    curNumCols=curRowLengths[y]-(curRows[y][curRowLengths[y]-1]=='\n');
	}
	SPI_freeString(added);
	caretPosition(AccessibleText_getCaretOffset(curTerm));
      } else
	LogPrint(LOG_INFO,"event %s, source %p, detail1 %lu detail2 %lu",event->type,event->source,event->detail1,event->detail2);
    }
    pthread_mutex_unlock(&updateMutex);
  }
  running = 0;
}

static void *doAtSpiScreenOpen(void *arg) {
  AccessibleEventListener *evListener;
  sem_t *SPI_init_sem = (sem_t *)arg;
  int res;
  static const char *events[] = {
    "object:text-changed",
    "object:text-caret-moved",
    "object:state-changed:focused",
    "focus:",
  };
  const char **event;
  if ((res=SPI_init())) {
    LogPrint(LOG_ERR,"SPI_init returned %d",res);
    return 0;
  }
  if (!(evListener = SPI_createAccessibleEventListener(evListenerCB,NULL)))
    LogPrint(LOG_ERR,"SPI_createAccessibleEventListener failed");
  else for (event=events; event<&events[sizeof(events)/sizeof(*events)]; event++)
    if (!(SPI_registerGlobalEventListener(evListener,*event)))
      LogPrint(LOG_ERR,"SPI_registerGlobalEventListener(%s) failed",*event);
  sem_post(SPI_init_sem);
  SPI_event_main();
  if (!(SPI_deregisterGlobalEventListenerAll(evListener)))
    LogPrint(LOG_ERR,"SPI_deregisterGlobalEventListenerAll failed");
  AccessibleEventListener_unref(evListener);
  if (curFocus)
    finiTerm();
  if ((res=SPI_exit()))
    LogPrint(LOG_ERR,"SPI_exit returned %d",res);
  return NULL;
}

static int
open_AtSpiScreen (void) {
  sem_t SPI_init_sem;
  sem_init(&SPI_init_sem,0,0);
  XtToolkitThreadInitialize();
  if (pthread_create(&SPI_main_thread,NULL,doAtSpiScreenOpen,(void *)&SPI_init_sem)) {
    LogPrint(LOG_ERR,"main SPI thread failed to be launched");
    return 0;
  }
  sem_wait(&SPI_init_sem);
  LogPrint(LOG_DEBUG,"SPI initialized");
  return 1;
}

static int
setup_AtSpiScreen (void) {
  return 1;
}

static void
close_AtSpiScreen (void) {
  SPI_event_quit();
  pthread_join(SPI_main_thread,NULL);
  LogPrint(LOG_DEBUG,"SPI stopped");
}

static int
selectvt_AtSpiScreen (int vt) {
  return 0;
}

static int
switchvt_AtSpiScreen (int vt) {
  return 0;
}

static int
currentvt_AtSpiScreen (void) {
  return curTerm? 0: -1;
}

static const char nonatspi [] = "not an AT-SPI terminal widget";

static void
describe_AtSpiScreen (ScreenDescription *description) {
  pthread_mutex_lock(&updateMutex);
  if (curTerm) {
    description->cols = curNumCols;
    description->rows = curNumRows?curNumRows:1;
    description->posx = curPosX;
    description->posy = curPosY;
  } else {
    description->rows = 1;
    description->cols = strlen(nonatspi);
    description->posx = 0;
    description->posy = 0;
  }
  pthread_mutex_unlock(&updateMutex);
  description->no = currentvt_AtSpiScreen();
}

static int
read_AtSpiScreen (ScreenBox box, unsigned char *buffer, ScreenMode mode) {
  long x,y;
  wchar_t c;
  if (box.height<0 || box.width<0) return 0;
  if (mode == SCR_ATTRIB) {
    memset(buffer,0x07,box.height*box.width);
    return 1;
  }
  memset(buffer,' ',box.height*box.width);
  pthread_mutex_lock(&updateMutex);
  if (!curTerm) {
    strncpy(buffer, nonatspi+box.left, box.width);
    goto out;
  }
  if (box.top+box.height>curNumRows)
    box.height = curNumRows-box.top;
  if (box.top<curNumRows && box.left<curNumCols)
    for (y=0; y<box.height; y++)
      if (curRowLengths[box.top+y])
	for (x=0; x<box.width; x++) {
	  if (box.left+x<curRowLengths[box.top+y] - (curRows[box.top+y][curRowLengths[box.top+y]-1]=='\n')) {
	    if ((c = curRows[box.top+y][box.left+x])<0x100)
	      /* latin1 only */
	      buffer[y*box.width+x] = c;
	    else
	      buffer[y*box.width+x] = '?';
	  }
	}
out:
  pthread_mutex_unlock(&updateMutex);
  return 1;
}

static int
insert_AtSpiScreen (ScreenKey key) {
  long keysym;
  int modMeta = 0, modControl=0;
  LogPrint(LOG_DEBUG, "Insert key: %4.4X", key);
  if (key < SCR_KEY_ENTER) {
    if (key & SCR_KEY_MOD_META) {
      key &= ~ SCR_KEY_MOD_META;
      modMeta = 1;
      SPI_generateKeyboardEvent(XK_Meta_L,NULL,SPI_KEY_SYM);
    }
    if (!(key & 0xE0)) {
      key |= 0x40;
      SPI_generateKeyboardEvent(XK_Control_L,NULL,SPI_KEY_SYM);
    }
  }

  switch (key) {
    case SCR_KEY_ENTER:         keysym = XK_KP_Enter;  break;
    case SCR_KEY_TAB:           keysym = XK_Tab;       break;
    case SCR_KEY_BACKSPACE:     keysym = XK_BackSpace; break;
    case SCR_KEY_ESCAPE:        keysym = XK_Escape;    break;
    case SCR_KEY_CURSOR_LEFT:   keysym = XK_Left;      break;
    case SCR_KEY_CURSOR_RIGHT:  keysym = XK_Right;     break;
    case SCR_KEY_CURSOR_UP:     keysym = XK_Up;        break;
    case SCR_KEY_CURSOR_DOWN:   keysym = XK_Down;      break;
    case SCR_KEY_PAGE_UP:       keysym = XK_Page_Up;   break;
    case SCR_KEY_PAGE_DOWN:     keysym = XK_Page_Down; break;
    case SCR_KEY_HOME:          keysym = XK_Home;      break;
    case SCR_KEY_END:           keysym = XK_End;       break;
    case SCR_KEY_INSERT:        keysym = XK_Insert;    break;
    case SCR_KEY_DELETE:        keysym = XK_Delete;    break;
    case SCR_KEY_FUNCTION + 0:  keysym = XK_F1;        break;
    case SCR_KEY_FUNCTION + 1:  keysym = XK_F2;        break;
    case SCR_KEY_FUNCTION + 2:  keysym = XK_F3;        break;
    case SCR_KEY_FUNCTION + 3:  keysym = XK_F4;        break;
    case SCR_KEY_FUNCTION + 4:  keysym = XK_F5;        break;
    case SCR_KEY_FUNCTION + 5:  keysym = XK_F6;        break;
    case SCR_KEY_FUNCTION + 6:  keysym = XK_F7;        break;
    case SCR_KEY_FUNCTION + 7:  keysym = XK_F8;        break;
    case SCR_KEY_FUNCTION + 8:  keysym = XK_F9;        break;
    case SCR_KEY_FUNCTION + 9:  keysym = XK_F10;       break;
    case SCR_KEY_FUNCTION + 10: keysym = XK_F11;       break;
    case SCR_KEY_FUNCTION + 11: keysym = XK_F12;       break;
    case SCR_KEY_FUNCTION + 12: keysym = XK_F13;       break;
    case SCR_KEY_FUNCTION + 13: keysym = XK_F14;       break;
    case SCR_KEY_FUNCTION + 14: keysym = XK_F15;       break;
    case SCR_KEY_FUNCTION + 15: keysym = XK_F16;       break;
    case SCR_KEY_FUNCTION + 16: keysym = XK_F17;       break;
    case SCR_KEY_FUNCTION + 17: keysym = XK_F18;       break;
    case SCR_KEY_FUNCTION + 18: keysym = XK_F19;       break;
    case SCR_KEY_FUNCTION + 19: keysym = XK_F20;       break;
    case SCR_KEY_FUNCTION + 20: keysym = XK_F21;       break;
    case SCR_KEY_FUNCTION + 21: keysym = XK_F22;       break;
    case SCR_KEY_FUNCTION + 22: keysym = XK_F23;       break;
    case SCR_KEY_FUNCTION + 23: keysym = XK_F24;       break;
    case SCR_KEY_FUNCTION + 24: keysym = XK_F25;       break;
    case SCR_KEY_FUNCTION + 25: keysym = XK_F26;       break;
    case SCR_KEY_FUNCTION + 26: keysym = XK_F27;       break;
    case SCR_KEY_FUNCTION + 27: keysym = XK_F28;       break;
    case SCR_KEY_FUNCTION + 28: keysym = XK_F29;       break;
    case SCR_KEY_FUNCTION + 29: keysym = XK_F30;       break;
    case SCR_KEY_FUNCTION + 30: keysym = XK_F31;       break;
    case SCR_KEY_FUNCTION + 31: keysym = XK_F32;       break;
    case SCR_KEY_FUNCTION + 32: keysym = XK_F33;       break;
    case SCR_KEY_FUNCTION + 33: keysym = XK_F34;       break;
    case SCR_KEY_FUNCTION + 34: keysym = XK_F35;       break;
    default: keysym = key; break;
  }

  SPI_generateKeyboardEvent(keysym,NULL,SPI_KEY_SYM);
  if (modControl)
    SPI_generateKeyboardEvent(XK_Control_L,NULL,SPI_KEY_SYM);
  if (modMeta)
    SPI_generateKeyboardEvent(XK_Meta_L,NULL,SPI_KEY_SYM);
  return 1;
}

static int
execute_AtSpiScreen (int command) {
  int blk = command & BRL_MSK_BLK;
  int arg
#ifdef HAVE_ATTRIBUTE_UNUSED
      __attribute__((unused))
#endif /* HAVE_ATTRIBUTE_UNUSED */
      = command & BRL_MSK_ARG;

  switch (blk) {
    case BRL_BLK_PASSAT2:
      break;
  }
  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_AtSpiScreen;
  main->base.read = read_AtSpiScreen;
  main->base.insert = insert_AtSpiScreen;
  main->base.selectvt = selectvt_AtSpiScreen;
  main->base.switchvt = switchvt_AtSpiScreen;
  main->base.currentvt = currentvt_AtSpiScreen;
  main->base.execute = execute_AtSpiScreen;
  main->prepare = prepare_AtSpiScreen;
  main->open = open_AtSpiScreen;
  main->setup = setup_AtSpiScreen;
  main->close = close_AtSpiScreen;
}
