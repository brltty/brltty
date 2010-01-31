/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2010 by The BRLTTY Developers.
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

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <limits.h>
#include <langinfo.h>
#include <locale.h>
#ifdef __MINGW32__
#include "win_pthread.h"
#else /* __MINGW32__ */
#include <semaphore.h>
#endif /* __MINGW32__ */

#include <dbus/dbus.h>
#define SPI2_DBUS_INTERFACE		"org.freedesktop.atspi"
#define SPI2_DBUS_INTERFACE_REG		SPI2_DBUS_INTERFACE".Registry"
#define SPI2_DBUS_PATH_DEC		"/org/freedesktop/atspi/registry/deviceeventcontroller"
#define SPI2_DBUS_INTERFACE_DEC		SPI2_DBUS_INTERFACE".DeviceEventController"
#define SPI2_DBUS_INTERFACE_EVENT	SPI2_DBUS_INTERFACE".Event"
#define SPI2_DBUS_INTERFACE_TREE	SPI2_DBUS_INTERFACE".Tree"
#define SPI2_DBUS_INTERFACE_TEXT	SPI2_DBUS_INTERFACE".Text"
#define SPI2_DBUS_INTERFACE_ACCESSIBLE	SPI2_DBUS_INTERFACE".Accessible"
#define SPI2_DBUS_INTERFACE_PROP	"org.freedesktop.DBus.Properties"

#include <X11/keysym.h>
#include <X11/Xlib.h>

#include "misc.h"
#include "brldefs.h"
#include "charset.h"

typedef enum {
  PARM_TYPE
} ScreenParameters;
#define SCRPARMS "type"

#include "scr_driver.h"

static int typeText = 1, typeTerminal = 1, typeAll = 0;
static char *curSender;
static char *curPath;

static long curNumRows, curNumCols;
static wchar_t **curRows;
static long *curRowLengths;
static long curCaret,curPosX,curPosY;
static pthread_mutex_t updateMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t SPI2_main_thread;

static DBusConnection *bus = NULL;

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
processParameters_AtSpi2Screen (char **parameters) {
  if (*parameters[PARM_TYPE]) {
    static const char *const choices[] = {"text"   , "terminal"   , "all"   , NULL}; 
    static       int  *const flags  [] = {&typeText, &typeTerminal, &typeAll, NULL};
    int count;
    char **types = splitString(parameters[PARM_TYPE], '+', &count);

    {
      int *const *flag = flags;
      while (*flag) **flag++ = 0;
    }

    {
      int index;
      for (index=0; index<count; index++) {
        const char *type = types[index];
        unsigned int choice;

        if (validateChoice(&choice, type, choices)) {
          int *flag = flags[choice];
          if ((flag == &typeAll) && (index > 0)) {
            LogPrint(LOG_WARNING, "widget type is mutually exclusive: %s", type);
          } else if (*flag || typeAll) {
            LogPrint(LOG_WARNING, "widget type specified more than once: %s", type);
          } else {
            *flag = 1;
          }
        } else {
          LogPrint(LOG_WARNING, "%s: %s", "invalid widget type", type);
        }
      }
    }

    deallocateStrings(types);
  }

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
  curCaret = caret;
}

static void finiTerm(void) {
  LogPrint(LOG_DEBUG,"end of term %s:%s",curSender,curPath);
  free(curSender);
  curSender = NULL;
  free(curPath);
  curPath = NULL;
  curPosX = curPosY = 0;
  free(curRows);
  curRows = NULL;
  curNumCols = curNumRows = 0;
}

/* Get the role of an AT-SPI2 object */
static char *getRole(const char *sender, const char *path) {
  const char *text;
  char *res = NULL;
  DBusError error;
  DBusMessage *msg, *reply;
  DBusMessageIter iter;

  dbus_error_init(&error);
  msg = dbus_message_new_method_call(sender, path, SPI2_DBUS_INTERFACE_ACCESSIBLE, "GetRoleName");
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while making getrole message: %s %s", error.name, error.message);
    dbus_error_free(&error);
    return NULL;
  }
  if (!msg) {
    LogPrint(LOG_DEBUG, "no memory while getting role");
    return NULL;
  }

  dbus_error_init(&error);
  /* 1s max delay */
  reply = dbus_connection_send_with_reply_and_block(bus, msg, 1000, &error);
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while getting role for %s:%s: %s %s", sender, path, error.name, error.message);
    dbus_error_free(&error);
    goto outMsg;
  }
  if (!reply) {
    LogPrint(LOG_DEBUG, "timeout while getting role");
    goto outMsg;
  }
  if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {
    LogPrint(LOG_DEBUG, "error while getting role");
    goto out;
  }
  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
    LogPrint(LOG_DEBUG, "GetRoleName didn't return a string but '%c'", dbus_message_iter_get_arg_type(&iter));
    goto out;
  }
  dbus_message_iter_get_basic(&iter, &text);
  res = strdup(text);

out:
  dbus_message_unref(reply);
outMsg:
  dbus_message_unref(msg);
  return res;
}

/* Get the text of an AT-SPI2 object */
static char *getText(const char *sender, const char *path) {
  const char *text;
  char *res = NULL;
  DBusError error;
  DBusMessage *msg, *reply;
  dbus_int32_t begin = 0;
  dbus_int32_t end = -1;
  DBusMessageIter iter;

  dbus_error_init(&error);
  msg = dbus_message_new_method_call(sender, path, SPI2_DBUS_INTERFACE_TEXT, "GetText");
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while making gettext message: %s %s", error.name, error.message);
    dbus_error_free(&error);
    return NULL;
  }
  if (!msg) {
    LogPrint(LOG_DEBUG, "no memory while getting text");
    return NULL;
  }
  dbus_message_append_args(msg, DBUS_TYPE_INT32, &begin, DBUS_TYPE_INT32, &end, DBUS_TYPE_INVALID);

  dbus_error_init(&error);
  /* 1s max delay */
  reply = dbus_connection_send_with_reply_and_block(bus, msg, 1000, &error);
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while getting text for %s:%s: %s %s", sender, path, error.name, error.message);
    dbus_error_free(&error);
    goto outMsg;
  }
  if (!reply) {
    LogPrint(LOG_DEBUG, "timeout while getting text");
    goto outMsg;
  }
  if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {
    LogPrint(LOG_DEBUG, "error while getting text");
    goto out;
  }
  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
    LogPrint(LOG_DEBUG, "GetText didn't return a string but '%c'", dbus_message_iter_get_arg_type(&iter));
    goto out;
  }
  dbus_message_iter_get_basic(&iter, &text);
  res = strdup(text);

out:
  dbus_message_unref(reply);
outMsg:
  dbus_message_unref(msg);
  return res;
}

/* Get the caret of an AT-SPI2 object */
static dbus_int32_t getCaret(const char *sender, const char *path) {
  dbus_int32_t res = -1;
  DBusError error;
  DBusMessage *msg, *reply = NULL;
  const char *interface = SPI2_DBUS_INTERFACE_TEXT;
  const char *property = "CaretOffset";
  DBusMessageIter iter, iter_variant;

  dbus_error_init(&error);
  msg = dbus_message_new_method_call(sender, path, SPI2_DBUS_INTERFACE_PROP, "Get");
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while making caret message: %s %s", error.name, error.message);
    dbus_error_free(&error);
    return -1;
  }
  if (!msg) {
    LogPrint(LOG_DEBUG, "no memory while making caret message");
    return -1;
  }
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID);

  dbus_error_init(&error);
  /* 1s max delay */
  reply = dbus_connection_send_with_reply_and_block(bus, msg, 1000, &error);
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while getting caret: %s %s", error.name, error.message);
    dbus_error_free(&error);
    goto outMsg;
  }
  if (!reply) {
    LogPrint(LOG_DEBUG, "timeout while getting caret");
    goto outMsg;
  }
  if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {
    LogPrint(LOG_DEBUG, "error while getting caret");
    goto out;
  }
  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    LogPrint(LOG_DEBUG, "getText didn't return a variant but '%c'", dbus_message_iter_get_arg_type(&iter));
    goto out;
  }
  dbus_message_iter_recurse(&iter, &iter_variant);
  if (dbus_message_iter_get_arg_type(&iter_variant) != DBUS_TYPE_INT32) {
    LogPrint(LOG_DEBUG, "getText didn't return an int32 but '%c'", dbus_message_iter_get_arg_type(&iter_variant));
    goto out;
  }
  dbus_message_iter_get_basic(&iter_variant, &res);
  LogPrint(LOG_DEBUG,"Got caret %d", res);

out:
  dbus_message_unref(reply);
outMsg:
  dbus_message_unref(msg);
  return res;
}

/* Switched to a new terminal, restart from scratch */
static void restartTerm(const char *sender, const char *path) {
  char *c,*d;
  const char *e;
  long i,len;
  char *text;

  if (curPath)
    finiTerm();

  text = getText(sender, path);
  if (!text)
    return;

  curSender = strdup(sender);
  curPath = strdup(path);
  LogPrint(LOG_DEBUG,"new term %s:%s with text %s",curSender,curPath, text);

  if (curRows) {
    for (i=0;i<curNumRows;i++)
      free(curRows[i]);
    free(curRows);
  }
  curNumRows = 0;
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
  caretPosition(getCaret(sender, path));
  free(text);
}

/* Handle incoming events */
static void AtSpi2HandleEvent(const char *interface, DBusMessage *message)
{
  DBusMessageIter iter, iter_variant;
  const char *detail;
  dbus_int32_t detail1, detail2;
  const char *member = dbus_message_get_member(message);
  const char *sender = dbus_message_get_sender(message);
  const char *path = dbus_message_get_path(message);
  int StateChanged_focused;

  dbus_message_iter_init(message, &iter);

  /* skip struct */
  dbus_message_iter_next(&iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
    LogPrint(LOG_DEBUG, "message detail not a string but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_get_basic(&iter, &detail);

  dbus_message_iter_next(&iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) {
    LogPrint(LOG_DEBUG, "message detail1 not an int32 but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_get_basic(&iter, &detail1);

  dbus_message_iter_next(&iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) {
    LogPrint(LOG_DEBUG, "message detail2 not an int32 but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_get_basic(&iter, &detail2);

  dbus_message_iter_next(&iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    LogPrint(LOG_DEBUG, "message detail2 not a variant but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_recurse(&iter, &iter_variant);
  //LogPrint(LOG_DEBUG, "event %s %s %s %s %s %d %d", interface, member, sender, path, detail, detail1, detail2);

  StateChanged_focused =
       !strcmp(interface, "Object")
    && !strcmp(member, "StateChanged")
    && !strcmp(detail, "focused");

  if (StateChanged_focused && !detail1) {
    if (curSender && !strcmp(sender, curSender) && !strcmp(path, curPath))
      finiTerm();
  } else if (!strcmp(interface,"Focus") || (StateChanged_focused && detail1)) {
    char *role = getRole(sender, path);
    LogPrint(LOG_DEBUG, "state changed focused to role %s", role);
    if (typeAll || (typeText && !strcmp(role, "text")) || (typeTerminal && !strcmp(role, "terminal"))) {
      restartTerm(sender, path);
    } else {
      if (curPath)
	finiTerm();
    }
    free(role);
  } else if (!strcmp(interface, "Object") && !strcmp(member, "TextCaretMoved")) {
    if (!curSender || strcmp(sender, curSender) || strcmp(path, curPath)) return;
    LogPrint(LOG_DEBUG, "caret move to %d", detail1);
    caretPosition(detail1);
  } else if (!strcmp(interface, "Object") && !strcmp(member, "TextChanged") && !strcmp(detail, "delete")) {
    long x,y,toDelete = detail2;
    long length = 0, toCopy;
    long downTo; /* line that will provide what will follow x */
    LogPrint(LOG_DEBUG,"delete %d from %d",detail2,detail1);
    if (!curSender || strcmp(sender, curSender) || strcmp(path, curPath)) return;
    findPosition(detail1,&x,&y);
    downTo = y;
    if (downTo < curNumRows)
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
	memmove(curRows[y]+x,curRows[downTo]+curRowLengths[downTo]-toCopy,toCopy*sizeof(*curRows[downTo]));
      if (y==downTo) {
	curRowLengths[y] = length-toDelete;
	curRows[y]=realloc(curRows[y],curRowLengths[y]*sizeof(*curRows[y]));
      }
    } else {
      /* kills this line as well ! */
      y--;
    }
    if (downTo>=curNumRows)
      /* imaginary extra lines don't need to be deleted */
      downTo=curNumRows-1;
    delRows(y+1,downTo-y);
  } else if (!strcmp(interface, "Object") && !strcmp(member, "TextChanged") && !strcmp(detail, "insert")) {
    long len=detail2,semilen,x,y;
    const char *added;
    const char *adding,*c;
    LogPrint(LOG_DEBUG,"insert %d from %d",detail2,detail1);
    if (!curSender || strcmp(sender, curSender) || strcmp(path, curPath)) return;
    findPosition(detail1,&x,&y);
    if (dbus_message_iter_get_arg_type(&iter_variant) != DBUS_TYPE_STRING) {
      LogPrint(LOG_DEBUG, "ergl, not string but '%c'", dbus_message_iter_get_arg_type(&iter_variant));
      return;
    }
    dbus_message_iter_get_basic(&iter_variant, &added);
    adding = c = added;
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
  } else {
      //LogPrint(LOG_DEBUG,"interface %s, member %s, detail %s, detail1 %d detail2 %d",interface, member, detail, detail1, detail2);
  }
}

static DBusHandlerResult AtSpi2Filter(DBusConnection *connection, DBusMessage *message, void *user_data)
{
  int type = dbus_message_get_type(message);
  const char *interface = dbus_message_get_interface(message);
  const char *member = dbus_message_get_member(message);
  if (type == DBUS_MESSAGE_TYPE_SIGNAL) {
    if (!strncmp(interface, SPI2_DBUS_INTERFACE_EVENT".", strlen(SPI2_DBUS_INTERFACE_EVENT".")))
      AtSpi2HandleEvent(interface + strlen(SPI2_DBUS_INTERFACE_EVENT"."), message);
    else
      LogPrint(LOG_DEBUG, "unknown signal %s %s", interface, member);
  } else
    LogPrint(LOG_DEBUG, "unknown message %d %s %s", type, interface, member);
  return DBUS_HANDLER_RESULT_HANDLED;
}

static int finished;

static void *doAtSpi2ScreenOpen(void *arg) {
  DBusError error;
  int res = 0;

  sem_t *SPI2_init_sem = (sem_t *)arg;

  dbus_error_init(&error);
  bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_ERR, "Can't get dbus session bus: %s %s", error.name, error.message);
    dbus_error_free(&error);
    goto out;
  }
  if (!bus) {
    LogPrint(LOG_ERR, "Can't get dbus session bus.");
    goto out;
  }
  if (!dbus_connection_add_filter(bus, AtSpi2Filter, NULL, NULL)) {
    goto outConn;
  }
#define WATCH(str) \
  dbus_error_init(&error); \
  dbus_bus_add_match(bus, str, &error); \
  if (dbus_error_is_set(&error)) { \
    LogPrint(LOG_ERR, "error while adding watch %s: %s %s", str, error.name, error.message); \
    dbus_error_free(&error); \
    goto out; \
  }
  WATCH("type='method_call',interface='"SPI2_DBUS_INTERFACE_TREE"'");
  WATCH("type='signal',interface='org.freedesktop.atspi.Event.Focus'");
  WATCH("type='signal',interface='org.freedesktop.atspi.Event.Object'");
  WATCH("type='signal',interface='org.freedesktop.atspi.Event.Object',member='TextChanged'");
  WATCH("type='signal',interface='org.freedesktop.atspi.Event.Object',member='TextCaretMoved'");
  WATCH("type='signal',interface='org.freedesktop.atspi.Event.Object',member='StateChanged'");

  res = 1;

  /* TODO: use dbus_watch_get_unix_fd() or dbus_watch_get_socket() instead */
  sem_post(SPI2_init_sem);
  while (!finished && dbus_connection_read_write_dispatch (bus, 1000))
    ;

  if (curPath)
    finiTerm();

  dbus_connection_remove_filter(bus, AtSpi2Filter, NULL);
outConn:
  dbus_connection_unref(bus);

out:
  return res;
}

static int
construct_AtSpi2Screen (void) {
  sem_t SPI2_init_sem;
  sem_init(&SPI2_init_sem,0,0);
  finished = 0;
  if (pthread_create(&SPI2_main_thread,NULL,doAtSpi2ScreenOpen,(void *)&SPI2_init_sem)) {
    LogPrint(LOG_ERR,"main SPI2 thread failed to be launched");
    return 0;
  }
  do {
    errno = 0;
  } while (sem_wait(&SPI2_init_sem) == -1 && errno == EINTR);
  if (errno) {
    LogError("SPI2 initialization wait failed");
    return 0;
  }
  LogPrint(LOG_DEBUG,"SPI2 initialized");
  return 1;
}

static void
destruct_AtSpi2Screen (void) {
  finished = 1;
  pthread_join(SPI2_main_thread,NULL);
  LogPrint(LOG_DEBUG,"SPI2 stopped");
}

static int
selectVirtualTerminal_AtSpi2Screen (int vt) {
  return 0;
}

static int
switchVirtualTerminal_AtSpi2Screen (int vt) {
  return 0;
}

static int
currentVirtualTerminal_AtSpi2Screen (void) {
  return curPath? 0: -1;
}

static const char nonatspi [] = "not an AT-SPI2 text widget";

static void
describe_AtSpi2Screen (ScreenDescription *description) {
  pthread_mutex_lock(&updateMutex);
  if (curPath) {
    description->cols = curNumCols;
    description->rows = curNumRows?curNumRows:1;
    description->posx = curPosX;
    description->posy = curPosY;
  } else {
    description->unreadable = nonatspi;
    description->rows = 1;
    description->cols = strlen(nonatspi);
    description->posx = 0;
    description->posy = 0;
  }
  pthread_mutex_unlock(&updateMutex);
  description->number = currentVirtualTerminal_AtSpi2Screen();
}

static int
readCharacters_AtSpi2Screen (const ScreenBox *box, ScreenCharacter *buffer) {
  long x,y;
  clearScreenCharacters(buffer,box->height*box->width);
  if (!curPath) {
    setScreenMessage(box, buffer, nonatspi);
    return 1;
  }
  if (!curNumCols || !curNumRows) return 0;
  if (!validateScreenBox(box, curNumCols, curNumRows)) return 0;
  pthread_mutex_lock(&updateMutex);
  for (y=0; y<box->height; y++) {
    if (curRowLengths[box->top+y]) {
      for (x=0; x<box->width; x++) {
        if (box->left+x<curRowLengths[box->top+y] - (curRows[box->top+y][curRowLengths[box->top+y]-1]=='\n')) {
          buffer[y*box->width+x].text = curRows[box->top+y][box->left+x];
        }
      }
    }
  }
out:
  pthread_mutex_unlock(&updateMutex);
  return 1;
}

enum key_type_e {
  PRESS,
  RELEASE,
  PRESSRELEASE,
  SYM
};

static int
AtSpi2GenerateKeyboardEvent (dbus_uint32_t keysym, enum key_type_e key_type)
{
  DBusError error;
  DBusMessage *msg, *reply;
  char *s = NULL;
  int res = 0;

  msg = dbus_message_new_method_call(SPI2_DBUS_INTERFACE_REG, SPI2_DBUS_PATH_DEC, SPI2_DBUS_INTERFACE_DEC, "GenerateKeyboardEvent");
  if (!msg) {
    LogPrint(LOG_DEBUG, "no memory while getting text");
    return 0;
  }
  dbus_message_append_args(msg, DBUS_TYPE_INT32, &keysym, DBUS_TYPE_STRING, &s, DBUS_TYPE_INT32, &key_type, DBUS_TYPE_INVALID);

  dbus_error_init(&error);
  /* 1s max delay */
  reply = dbus_connection_send_with_reply_and_block(bus, msg, 1000, &error);
  if (dbus_error_is_set(&error)) {
    LogPrint(LOG_DEBUG, "error while generating keyboard event: %s %s", error.name, error.message);
    goto outMsg;
  }
  if (!reply) {
    LogPrint(LOG_DEBUG, "timeout while generating keyboard event");
    goto outMsg;
  }
  res = 1;

outMsg:
  dbus_message_unref(msg);
  return res;
}

static int
insertKey_AtSpi2Screen (ScreenKey key) {
  long keysym;
  int modMeta=0, modControl=0;

  setKeyModifiers(&key, SCR_KEY_CONTROL);

  if (isSpecialKey(key)) {
    switch (key & SCR_KEY_CHAR_MASK) {
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
      default: LogPrint(LOG_WARNING, "key not insertable: %04X", key); return 0;
    }
  } else {
    wchar_t wc;

    if (key & SCR_KEY_ALT_LEFT) {
      key &= ~SCR_KEY_ALT_LEFT;
      modMeta = 1;
    }

    if (key & SCR_KEY_CONTROL) {
      key &= ~SCR_KEY_CONTROL;
      modControl = 1;
    }

    wc = key & SCR_KEY_CHAR_MASK;
    if (wc < 0x100)
      keysym = wc; /* latin1 character */
    else
      keysym = 0x1000000 | wc;
  }
  LogPrint(LOG_DEBUG, "inserting key: %04X -> %s%s%ld",
           key,
           (modMeta? "meta ": ""),
           (modControl? "control ": ""),
           keysym);

  {
    int ok = 0;

    if (!modMeta || AtSpi2GenerateKeyboardEvent(XK_Meta_L,PRESS)) {
      if (!modControl || AtSpi2GenerateKeyboardEvent(XK_Control_L,PRESS)) {
        if (AtSpi2GenerateKeyboardEvent(keysym,SYM)) {
          ok = 1;
        } else {
          LogPrint(LOG_WARNING, "key insertion failed.");
        }

        if (modControl && !AtSpi2GenerateKeyboardEvent(XK_Control_L,RELEASE)) {
          LogPrint(LOG_WARNING, "control release failed.");
          ok = 0;
        }
      } else {
        LogPrint(LOG_WARNING, "control press failed.");
      }

      if (modMeta && !AtSpi2GenerateKeyboardEvent(XK_Meta_L,RELEASE)) {
        LogPrint(LOG_WARNING, "meta release failed.");
        ok = 0;
      }
    } else {
      LogPrint(LOG_WARNING, "meta press failed.");
    }

    return ok;
  }
}

static int
executeCommand_AtSpi2Screen (int command) {
  int blk UNUSED = command & BRL_MSK_BLK;
  int arg UNUSED = command & BRL_MSK_ARG;

  return 0;
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.describe = describe_AtSpi2Screen;
  main->base.readCharacters = readCharacters_AtSpi2Screen;
  main->base.insertKey = insertKey_AtSpi2Screen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_AtSpi2Screen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_AtSpi2Screen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_AtSpi2Screen;
  main->base.executeCommand = executeCommand_AtSpi2Screen;
  main->processParameters = processParameters_AtSpi2Screen;
  main->construct = construct_AtSpi2Screen;
  main->destruct = destruct_AtSpi2Screen;
}
