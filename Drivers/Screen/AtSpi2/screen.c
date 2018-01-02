/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>

#ifdef HAVE_ATSPI_GET_A11Y_BUS
#include <atspi/atspi.h>
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */

#ifdef __MINGW32__
#include "win_pthread.h"
#else /* __MINGW32__ */
#include <semaphore.h>
#endif /* __MINGW32__ */

#include <dbus/dbus.h>
#define SPI2_DBUS_INTERFACE		"org.a11y.atspi"
#define SPI2_DBUS_INTERFACE_REG		SPI2_DBUS_INTERFACE".Registry"
#define SPI2_DBUS_PATH_REG		"/org/a11y/atspi/registry"
#define SPI2_DBUS_PATH_ROOT		"/org/a11y/atspi/accessible/root"
#define SPI2_DBUS_PATH_DEC		SPI2_DBUS_PATH_REG "/deviceeventcontroller"
#define SPI2_DBUS_INTERFACE_DEC		SPI2_DBUS_INTERFACE".DeviceEventController"
#define SPI2_DBUS_INTERFACE_DEL		SPI2_DBUS_INTERFACE".DeviceEventListener"
#define SPI2_DBUS_INTERFACE_EVENT	SPI2_DBUS_INTERFACE".Event"
#define SPI2_DBUS_INTERFACE_TREE	SPI2_DBUS_INTERFACE".Tree"
#define SPI2_DBUS_INTERFACE_TEXT	SPI2_DBUS_INTERFACE".Text"
#define SPI2_DBUS_INTERFACE_ACCESSIBLE	SPI2_DBUS_INTERFACE".Accessible"
#define FREEDESKTOP_DBUS_INTERFACE	"org.freedesktop.DBus"
#define FREEDESKTOP_DBUS_INTERFACE_PROP	FREEDESKTOP_DBUS_INTERFACE".Properties"


#define ATSPI_STATE_ACTIVE 1
#define ATSPI_STATE_FOCUSED 12

#ifdef HAVE_X11_KEYSYM_H
#include <X11/keysym.h>
#endif /* HAVE_X11_KEYSYM_H */

#include "log.h"
#include "parse.h"
#include "thread.h"
#include "brl_cmds.h"
#include "charset.h"
#include "async_io.h"
#include "async_alarm.h"
#include "async_event.h"

typedef enum {
  PARM_RELEASE,
  PARM_TYPE
} ScreenParameters;
#define SCRPARMS "release", "type"

#include "scr_driver.h"

typedef enum {
  TYPE_TEXT,
  TYPE_TERMINAL,
  TYPE_ALL,
  TYPE_COUNT
} TypeValue;

static unsigned int releaseScreen;
static unsigned char typeFlags[TYPE_COUNT];

static char *curSender;
static char *curPath;

static long curNumRows, curNumCols;
static wchar_t **curRows;
static long *curRowLengths;
static long curCaret,curPosX,curPosY;

static DBusConnection *bus = NULL;

static int updated;

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
    if ((ssize_t)(eaten = my_mbrlen(s,n,&ps))<0)
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
  releaseScreen = 1;
  {
    const char *parameter = parameters[PARM_RELEASE];

    if (*parameter) {
      if (!validateYesNo(&releaseScreen, parameter)) {
        logMessage(LOG_WARNING, "invalid release screen setting: %s", parameter);
      }
    }
  }

  typeFlags[TYPE_TEXT] = 1;
  typeFlags[TYPE_TERMINAL] = 1;
  typeFlags[TYPE_ALL] = 0;
  {
    const char *parameter = parameters[PARM_TYPE];

    if (*parameter) {
      int count;
      char **types = splitString(parameter, '+', &count);

      if (types) {
        static const char *const choices[] = {
          [TYPE_TEXT] = "text",
          [TYPE_TERMINAL] = "terminal",
          [TYPE_ALL] = "all",
          [TYPE_COUNT] = NULL
        };

        for (unsigned int index=0; index<TYPE_COUNT; index+=1) {
          typeFlags[index] = 0;
        }

        for (unsigned int index=0; index<count; index+=1) {
          const char *type = types[index];
          unsigned int choice;

          if (validateChoice(&choice, type, choices)) {
            if ((choice == TYPE_ALL) && (index > 0)) {
              logMessage(LOG_WARNING, "widget type is mutually exclusive: %s", type);
            } else if (typeFlags[choice] || typeFlags[TYPE_ALL]) {
              logMessage(LOG_WARNING, "widget type specified more than once: %s", type);
            } else {
              typeFlags[choice] = 1;
            }
          } else {
            logMessage(LOG_WARNING, "%s: %s", "invalid widget type", type);
          }
        }

        deallocateStrings(types);
      }
    }
  }

  return 1;
}

/* Creates a method call message */
static DBusMessage *
new_method_call(const char *sender, const char *path, const char *interface, const char *method)
{
  DBusError error;
  DBusMessage *msg;

  dbus_error_init(&error);
  msg = dbus_message_new_method_call(sender, path, interface, method);
  if (dbus_error_is_set(&error)) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "error while making %s message: %s %s", method, error.name, error.message);

    dbus_error_free(&error);
    return NULL;
  }
  if (!msg) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "no memory while making %s message", method);
    return NULL;
  }
  return msg;
}

/* Sends a method call message, and returns the reply, if any. This unrefs the message.  */
static DBusMessage *
send_with_reply_and_block(DBusConnection *bus, DBusMessage *msg, int timeout_ms, const char *doing)
{
  DBusError error;
  DBusMessage *reply;

  dbus_error_init(&error);
  reply = dbus_connection_send_with_reply_and_block(bus, msg, timeout_ms, &error);
  dbus_message_unref(msg);
  if (dbus_error_is_set(&error)) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "error while %s: %s %s", doing, error.name, error.message);
    dbus_error_free(&error);
    return NULL;
  }
  if (!reply) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "timeout while %s", doing);
    return NULL;
  }
  if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "error while %s", doing);
    dbus_message_unref(reply);
    return NULL;
  }
  return reply;
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
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "end of term %s:%s",curSender,curPath);
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
  DBusMessage *msg, *reply;
  DBusMessageIter iter;

  msg = new_method_call(sender, path, SPI2_DBUS_INTERFACE_ACCESSIBLE, "GetRoleName");
  if (!msg)
    return NULL;
  reply = send_with_reply_and_block(bus, msg, 1000, "getting role");
  if (!reply)
    return NULL;

  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "GetRoleName didn't return a string but '%c'", dbus_message_iter_get_arg_type(&iter));
    goto out;
  }
  dbus_message_iter_get_basic(&iter, &text);
  res = strdup(text);

out:
  dbus_message_unref(reply);
  return res;
}

/* Get the text of an AT-SPI2 object */
static char *getText(const char *sender, const char *path) {
  const char *text;
  char *res = NULL;
  DBusMessage *msg, *reply;
  dbus_int32_t begin = 0;
  dbus_int32_t end = -1;
  DBusMessageIter iter;

  msg = new_method_call(sender, path, SPI2_DBUS_INTERFACE_TEXT, "GetText");
  if (!msg)
    return NULL;
  dbus_message_append_args(msg, DBUS_TYPE_INT32, &begin, DBUS_TYPE_INT32, &end, DBUS_TYPE_INVALID);
  reply = send_with_reply_and_block(bus, msg, 1000, "getting text");
  if (!reply)
    return NULL;

  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "GetText didn't return a string but '%c'", dbus_message_iter_get_arg_type(&iter));
    goto out;
  }
  dbus_message_iter_get_basic(&iter, &text);
  res = strdup(text);

out:
  dbus_message_unref(reply);
  return res;
}

/* Get the caret of an AT-SPI2 object */
static dbus_int32_t getCaret(const char *sender, const char *path) {
  dbus_int32_t res = -1;
  DBusMessage *msg, *reply = NULL;
  const char *interface = SPI2_DBUS_INTERFACE_TEXT;
  const char *property = "CaretOffset";
  DBusMessageIter iter, iter_variant;

  msg = new_method_call(sender, path, FREEDESKTOP_DBUS_INTERFACE_PROP, "Get");
  if (!msg)
    return -1;
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID);
  reply = send_with_reply_and_block(bus, msg, 1000, "getting caret");
  if (!reply)
    return -1;

  dbus_message_iter_init(reply, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "getText didn't return a variant but '%c'", dbus_message_iter_get_arg_type(&iter));
    goto out;
  }
  dbus_message_iter_recurse(&iter, &iter_variant);
  if (dbus_message_iter_get_arg_type(&iter_variant) != DBUS_TYPE_INT32) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "getText didn't return an int32 but '%c'", dbus_message_iter_get_arg_type(&iter_variant));
    goto out;
  }
  dbus_message_iter_get_basic(&iter_variant, &res);
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "Got caret %d", res);

out:
  dbus_message_unref(reply);
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
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "new term %s:%s with text %s",curSender,curPath, text);

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
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "%ld rows",curNumRows);
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
	logMessage(LOG_ERR,"unterminated sequence %s",c);
      else if (len==-1)
	logSystemError("mbrlen");
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
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "%ld cols",curNumCols);
  caretPosition(getCaret(sender, path));
  free(text);
}

/* Switched to a new object, check whether we want to read it, and if so, restart with it */
static void tryRestartTerm(const char *sender, const char *path) {
  char *role = getRole(sender, path);
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "state changed focus to role %s", role);
  if (typeFlags[TYPE_ALL] ||
      (role && typeFlags[TYPE_TEXT] && (strcmp(role, "text") == 0)) ||
      (role && typeFlags[TYPE_TERMINAL] && (strcmp(role, "terminal") == 0))) {
    restartTerm(sender, path);
  } else {
    if (curPath)
      finiTerm();
  }
  free(role);
}

/* Get the state of an object */
static dbus_uint32_t *getState(const char *sender, const char *path)
{
  DBusMessage *msg, *reply;
  DBusMessageIter iter, iter_array;
  dbus_uint32_t *states, *ret = NULL;
  int count;

  msg = new_method_call(sender, path, SPI2_DBUS_INTERFACE_ACCESSIBLE, "GetState");
  if (!msg)
    return NULL;
  reply = send_with_reply_and_block(bus, msg, 1000, "getting state");
  if (!reply)
    return NULL;

  if (strcmp (dbus_message_get_signature (reply), "au") != 0)
  {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "unexpected signature %s while getting active state", dbus_message_get_signature(reply));
    goto out;
  }
  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &iter_array);
  dbus_message_iter_get_fixed_array (&iter_array, &states, &count);
  if (count != 2)
  {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "unexpected signature %s while getting active state", dbus_message_get_signature(reply));
    goto out;
  }
  ret = malloc(sizeof(*ret) * count);
  memcpy(ret, states, sizeof(*ret) * count);

out:
  dbus_message_unref(reply);
  return ret;
}

/* Check whether an ancestor of this object is active */
static int checkActiveParent(const char *sender, const char *path) {
  DBusMessage *msg, *reply;
  DBusMessageIter iter, iter_variant, iter_struct;
  int res = 0;
  const char *interface = SPI2_DBUS_INTERFACE_ACCESSIBLE;
  const char *property = "Parent";
  dbus_uint32_t *states;

  msg = new_method_call(sender, path, FREEDESKTOP_DBUS_INTERFACE_PROP, "Get");
  if (!msg)
    return 0;
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID);
  reply = send_with_reply_and_block(bus, msg, 1000, "checking active object");
  if (!reply)
    return 0;

  if (strcmp (dbus_message_get_signature (reply), "v") != 0)
  {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "unexpected signature %s while checking active object", dbus_message_get_signature(reply));
    goto out;
  }

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &iter_variant);
  dbus_message_iter_recurse (&iter_variant, &iter_struct);
  dbus_message_iter_get_basic (&iter_struct, &sender);
  dbus_message_iter_next (&iter_struct);
  dbus_message_iter_get_basic (&iter_struct, &path);

  states = getState(sender, path);
  if (states) {
    res = (states[0] & (1<<ATSPI_STATE_ACTIVE)) != 0 || checkActiveParent(sender, path);
    free(states);
  } else {
    res = 0;
  }

out:
  dbus_message_unref(reply);
  return res;
}

/* Check whether this object is the focused object (which is way faster than
 * browsing all objects of the desktop) */
static int reinitTerm(const char *sender, const char *path) {
  dbus_uint32_t *states = getState(sender, path);
  int active = 0;

  if (!states)
    return 0;

  /* Whether this widget is active */
  active = (states[0] & (1<<ATSPI_STATE_ACTIVE)) != 0;

  if (states[0] & (1<<ATSPI_STATE_FOCUSED)) {
    free(states);
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "%s %s is focused!", sender, path);
    /* This widget is focused */
    if (active) {
      /* And it is active, we are done.  */
      tryRestartTerm(sender, path);
      return 1;
    } else {
      /* Check that a parent is active.  */
      return checkActiveParent(sender, path);
    }
  }

  free(states);
  return 0;
}

/* Try to find an active object among children of the given object */
static int findTerm(const char *sender, const char *path, int active, int depth);
static int recurseFindTerm(const char *sender, const char *path, int active, int depth) {
  DBusMessage *msg, *reply;
  DBusMessageIter iter, iter_array, iter_struct;
  int res = 0;

  msg = new_method_call(sender, path, SPI2_DBUS_INTERFACE_ACCESSIBLE, "GetChildren");
  if (!msg)
    return 0;
  reply = send_with_reply_and_block(bus, msg, 1000, "getting active object");
  if (!reply)
    return 0;

  if (strcmp (dbus_message_get_signature (reply), "a(so)") != 0)
  {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "unexpected signature %s while getting active object", dbus_message_get_signature(reply));
    goto out;
  }
  dbus_message_iter_init(reply, &iter);
  dbus_message_iter_recurse (&iter, &iter_array);
  while (dbus_message_iter_get_arg_type (&iter_array) != DBUS_TYPE_INVALID)
  {
    const char *childsender, *childpath;
    dbus_message_iter_recurse (&iter_array, &iter_struct);
    dbus_message_iter_get_basic (&iter_struct, &childsender);
    dbus_message_iter_next (&iter_struct);
    dbus_message_iter_get_basic (&iter_struct, &childpath);
    /* Make sure that the child is not the same as the parent, to avoid recursing indefinitely.  */
    if (strcmp(path, childpath))
    {
      if (findTerm(childsender, childpath, active, depth))
      {
	res = 1;
	goto out;
      }
    }
    dbus_message_iter_next (&iter_array);
  }

out:
  dbus_message_unref(reply);
  return res;
}

/* Test whether this object is active, and if not recurse in its children */
static int findTerm(const char *sender, const char *path, int active, int depth) {
  dbus_uint32_t *states = getState(sender, path);

  if (!states)
    return 0;

  if (states[0] & (1<<ATSPI_STATE_ACTIVE))
    /* This application is active */
    active = 1;

  if (states[0] & (1<<ATSPI_STATE_FOCUSED) && active)
  {
    /* And this widget is focused */
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "%s %s is focused!", sender, path);
    free(states);
    tryRestartTerm(sender, path);
    return 1;
  }

  free(states);
  return recurseFindTerm(sender, path, active, depth+1);
}

/* Find out currently focused terminal, starting from registry */
static void initTerm(void) {
  recurseFindTerm(SPI2_DBUS_INTERFACE_REG, SPI2_DBUS_PATH_ROOT, 0, 0);
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

  if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRUCT) {
    /* skip struct */
    dbus_message_iter_next(&iter);
  }

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "message detail not a string but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_get_basic(&iter, &detail);

  dbus_message_iter_next(&iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "message detail1 not an int32 but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_get_basic(&iter, &detail1);

  dbus_message_iter_next(&iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "message detail2 not an int32 but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_get_basic(&iter, &detail2);

  dbus_message_iter_next(&iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "message detail2 not a variant but '%c'", dbus_message_iter_get_arg_type(&iter));
    return;
  }
  dbus_message_iter_recurse(&iter, &iter_variant);

  StateChanged_focused =
       !strcmp(interface, "Object")
    && !strcmp(member, "StateChanged")
    && !strcmp(detail, "focused");

  if (StateChanged_focused && !detail1) {
    if (curSender && !strcmp(sender, curSender) && !strcmp(path, curPath))
      finiTerm();
  } else if (!strcmp(interface,"Focus") || (StateChanged_focused && detail1)) {
    tryRestartTerm(sender, path);
  } else if (!strcmp(interface, "Object") && !strcmp(member, "TextCaretMoved")) {
    if (!curSender || strcmp(sender, curSender) || strcmp(path, curPath)) return;
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "caret move to %d", detail1);
    caretPosition(detail1);
  } else if (!strcmp(interface, "Object") && !strcmp(member, "TextChanged") && !strcmp(detail, "delete")) {
    long x,y,toDelete = detail2;
    const char *deleted;
    long length = 0, toCopy;
    long downTo; /* line that will provide what will follow x */
    if (!curSender || strcmp(sender, curSender) || strcmp(path, curPath)) return;
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "delete %d from %d",detail2,detail1);
    findPosition(detail1,&x,&y);
    if (dbus_message_iter_get_arg_type(&iter_variant) != DBUS_TYPE_STRING) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                 "ergl, not string but '%c'", dbus_message_iter_get_arg_type(&iter_variant));
      return;
    }
    dbus_message_iter_get_basic(&iter_variant, &deleted);
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "'%s'",deleted);
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
	  logMessage(LOG_ERR,"deleting past end of text !");
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
    caretPosition(curCaret);
  } else if (!strcmp(interface, "Object") && !strcmp(member, "TextChanged") && !strcmp(detail, "insert")) {
    long len=detail2,semilen,x,y;
    const char *added;
    const char *adding,*c;
    if (!curSender || strcmp(sender, curSender) || strcmp(path, curPath)) return;
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "insert %d from %d",detail2,detail1);
    findPosition(detail1,&x,&y);
    if (dbus_message_iter_get_arg_type(&iter_variant) != DBUS_TYPE_STRING) {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                 "ergl, not string but '%c'", dbus_message_iter_get_arg_type(&iter_variant));
      return;
    }
    dbus_message_iter_get_basic(&iter_variant, &added);
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "'%s'",added);
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
    caretPosition(curCaret);
  } else {
    return;
  }
  updated = 1;
}

static DBusHandlerResult AtSpi2Filter(DBusConnection *connection, DBusMessage *message, void *user_data)
{
  int type = dbus_message_get_type(message);
  const char *interface = dbus_message_get_interface(message);
  const char *member = dbus_message_get_member(message);

  if (type == DBUS_MESSAGE_TYPE_SIGNAL) {
    if (!strncmp(interface, SPI2_DBUS_INTERFACE_EVENT".", strlen(SPI2_DBUS_INTERFACE_EVENT"."))) {
      AtSpi2HandleEvent(interface + strlen(SPI2_DBUS_INTERFACE_EVENT"."), message);
    } else {
      logMessage(LOG_CATEGORY(SCREEN_DRIVER),
                 "unknown signal: Intf:%s Msg:%s", interface, member);
    }
  } else {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "unknown message: Type:%d Intf:%s Msg:%s", type, interface, member);
  }

  return DBUS_HANDLER_RESULT_HANDLED;
}

/* Integration of DBus watches with brltty monitors */

struct a2Watch
{
  AsyncHandle input_monitor;
  AsyncHandle output_monitor;
  DBusWatch *watch;
};

int a2ProcessWatch(const AsyncMonitorCallbackParameters *parameters, int flags)
{
  struct a2Watch *a2Watch = parameters->data;
  DBusWatch *watch = a2Watch->watch;
  /* Read/Write on socket */
  dbus_watch_handle(watch, parameters->error?DBUS_WATCH_ERROR:flags);
  /* And process messages */
  while (dbus_connection_dispatch(bus) != DBUS_DISPATCH_COMPLETE)
    ;
  if (updated)
  {
    updated = 0;
    mainScreenUpdated();
  }
  return dbus_watch_get_enabled(watch);
}

ASYNC_MONITOR_CALLBACK(a2ProcessInput) {
  if (a2ProcessWatch(parameters, DBUS_WATCH_READABLE)) return 1;

  struct a2Watch *a2Watch = parameters->data;
  asyncDiscardHandle(a2Watch->input_monitor);
  a2Watch->input_monitor = NULL;
  return 0;
}

ASYNC_MONITOR_CALLBACK(a2ProcessOutput) {
  if (a2ProcessWatch(parameters, DBUS_WATCH_WRITABLE)) return 1;

  struct a2Watch *a2Watch = parameters->data;
  asyncDiscardHandle(a2Watch->output_monitor);
  a2Watch->output_monitor = NULL;
  return 0;
}

dbus_bool_t a2AddWatch(DBusWatch *watch, void *data)
{
  struct a2Watch *a2Watch = calloc(1, sizeof(*a2Watch));
  a2Watch->watch = watch;
  int flags = dbus_watch_get_flags(watch);
  if (dbus_watch_get_enabled(watch))
  {
    if (flags & DBUS_WATCH_READABLE)
      asyncMonitorFileInput(&a2Watch->input_monitor, dbus_watch_get_unix_fd(watch), a2ProcessInput, a2Watch);
    if (flags & DBUS_WATCH_WRITABLE)
      asyncMonitorFileOutput(&a2Watch->output_monitor, dbus_watch_get_unix_fd(watch), a2ProcessOutput, a2Watch);
  }
  dbus_watch_set_data(watch, a2Watch, NULL);
  return TRUE;
}

void a2RemoveWatch(DBusWatch *watch, void *data)
{
  struct a2Watch *a2Watch = dbus_watch_get_data(watch);
  dbus_watch_set_data(watch, NULL, NULL);
  if (a2Watch->input_monitor)
    asyncCancelRequest(a2Watch->input_monitor);
  if (a2Watch->output_monitor)
    asyncCancelRequest(a2Watch->output_monitor);
  free(a2Watch);
}

void a2WatchToggled(DBusWatch *watch, void *data)
{
  if (dbus_watch_get_enabled(watch)) {
    if (!dbus_watch_get_data(watch))
      a2AddWatch(watch, data);
  } else {
    if (dbus_watch_get_data(watch))
      a2RemoveWatch(watch, data);
  }
}

/* Integration of DBus timeouts with brltty monitors */

struct a2Timeout
{
  AsyncHandle monitor;
  DBusTimeout *timeout;
};

ASYNC_ALARM_CALLBACK(a2ProcessTimeout)
{
  struct a2Timeout *a2Timeout = parameters->data;
  DBusTimeout *timeout = a2Timeout->timeout;
  /* Process timeout */
  dbus_timeout_handle(timeout);
  /* And process messages */
  while (dbus_connection_dispatch(bus) != DBUS_DISPATCH_COMPLETE)
    ;
  if (updated)
  {
    updated = 0;
    mainScreenUpdated();
  }
  asyncDiscardHandle(a2Timeout->monitor);
  a2Timeout->monitor = NULL;
  if (dbus_timeout_get_enabled(timeout))
    /* Still enabled, requeue it */
    asyncNewRelativeAlarm(&a2Timeout->monitor, dbus_timeout_get_interval(timeout), a2ProcessTimeout, a2Timeout);
}

dbus_bool_t a2AddTimeout(DBusTimeout *timeout, void *data)
{
  struct a2Timeout *a2Timeout = calloc(1, sizeof(*a2Timeout));
  a2Timeout->timeout = timeout;
  if (dbus_timeout_get_enabled(timeout))
    asyncNewRelativeAlarm(&a2Timeout->monitor, dbus_timeout_get_interval(timeout), a2ProcessTimeout, a2Timeout);
  dbus_timeout_set_data(timeout, a2Timeout, NULL);
  return TRUE;
}

void a2RemoveTimeout(DBusTimeout *timeout, void *data)
{
  struct a2Timeout *a2Timeout = dbus_timeout_get_data(timeout);
  dbus_timeout_set_data(timeout, NULL, NULL);
  if (a2Timeout->monitor)
    asyncCancelRequest(a2Timeout->monitor);
  free(a2Timeout);
}

void a2TimeoutToggled(DBusTimeout *timeout, void *data)
{
  if (dbus_timeout_get_enabled(timeout)) {
    if (!dbus_timeout_get_data(timeout))
      a2AddTimeout(timeout, data);
  } else {
    if (dbus_timeout_get_data(timeout))
      a2RemoveTimeout(timeout, data);
  }
}

/* Driver construction / destruction */

static int addWatch(const char *message, const char *event) {
  DBusError error;
  DBusMessage *msg, *reply;

  dbus_error_init(&error);
  dbus_bus_add_match(bus, message, &error);
  if (dbus_error_is_set(&error)) {
    logMessage(LOG_ERR, "error while adding watch %s: %s %s", message, error.name, error.message);
    dbus_error_free(&error);
    return 0;
  }
 
  if (!event)
    return 1;

  /* Register as event listener. */
  msg = new_method_call(SPI2_DBUS_INTERFACE_REG, SPI2_DBUS_PATH_REG, SPI2_DBUS_INTERFACE_REG, "RegisterEvent");
  if (!msg)
    return 0;
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &event, DBUS_TYPE_INVALID);
  reply = send_with_reply_and_block(bus, msg, 1000, "registering listener");
  if (!reply)
    return 0;

  dbus_message_unref(reply);
  return 1;
}

static int
addWatches (void) {
  typedef struct {
    const char *message;
    const char *event;
  } WatchEntry;

  static const WatchEntry watchTable[] = {
    { .message = "type='method_call',interface='"SPI2_DBUS_INTERFACE_TREE"'",
      .event = NULL
    },

    { .message = "type='signal',interface='"SPI2_DBUS_INTERFACE_EVENT".Focus'",
      .event = "focus"
    },

    { .message = "type='signal',interface='"SPI2_DBUS_INTERFACE_EVENT".Object'",
      .event = "object"
    },

    { .message = "type='signal',interface='"SPI2_DBUS_INTERFACE_EVENT".Object',member='ChildrenChanged'",
      .event = "object:childrenchanged"
    },

    { .message = "type='signal',interface='"SPI2_DBUS_INTERFACE_EVENT".Object',member='TextChanged'",
      .event = "object:textchanged"
    },

    { .message = "type='signal',interface='"SPI2_DBUS_INTERFACE_EVENT".Object',member='TextCaretMoved'",
      .event = "object:textcaretmoved"
    },

    { .message = "type='signal',interface='"SPI2_DBUS_INTERFACE_EVENT".Object',member='StateChanged'",
      .event = "object:statechanged"
    },

    { .message = NULL }
  };

  for (const WatchEntry *watch=watchTable; watch->message; watch+=1) {
    if (!addWatch(watch->message, watch->event)) return 0;
  }

  return 1;
}

static int
construct_AtSpi2Screen (void) {
  DBusError error;

  dbus_error_init(&error);
#ifdef HAVE_ATSPI_GET_A11Y_BUS
  bus = atspi_get_a11y_bus();
  if (!bus)
#endif
  {
    bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error)) {
      logMessage(LOG_ERR, "can't get dbus session bus: %s %s", error.name, error.message);
      dbus_error_free(&error);
      goto noBus;
    }
  }

  if (!bus) {
    logMessage(LOG_ERR, "can't get dbus session bus");
    goto noBus;
  }

  if (!dbus_connection_add_filter(bus, AtSpi2Filter, NULL, NULL)) goto noConnection;
  if (!addWatches()) goto noWatches;

  if (!curPath) {
    initTerm();
  } else if (!reinitTerm(curSender, curPath)) {
    logMessage(LOG_CATEGORY(SCREEN_DRIVER),
               "caching failed, restarting from scratch");
    initTerm();
  }

  dbus_connection_set_watch_functions(bus, a2AddWatch, a2RemoveWatch, a2WatchToggled, NULL, NULL);
  dbus_connection_set_timeout_functions(bus, a2AddTimeout, a2RemoveTimeout, a2TimeoutToggled, NULL, NULL);

  logMessage(LOG_CATEGORY(SCREEN_DRIVER), "SPI2 initialized");
  return 1;

noWatches:

noConnection:
  dbus_connection_unref(bus);

noBus:
  return 0;
}

static void
destruct_AtSpi2Screen (void) {
  dbus_connection_remove_filter(bus, AtSpi2Filter, NULL);
  dbus_connection_close(bus);
  dbus_connection_unref(bus);
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "SPI2 stopped");
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
  return (curPath || !releaseScreen)? 0: SCR_NO_VT;
}

static const char nonatspi [] = "not an AT-SPI2 text widget";

static int
poll_AtSpi2Screen (void)
{
  return 0;
}

ASYNC_EVENT_CALLBACK(AtSpi2ScreenUpdated) {
  mainScreenUpdated();
}

static int
refresh_AtSpi2Screen (void)
{
  return 1;
}

static void
describe_AtSpi2Screen (ScreenDescription *description) {
  if (curPath) {
    description->cols = curPosX>=curNumCols?curPosX+1:curNumCols;
    description->rows = curNumRows?curNumRows:1;
    description->posx = curPosX;
    description->posy = curPosY;
  } else {
    const char *message = nonatspi;
    if (releaseScreen) description->unreadable = message;

    description->rows = 1;
    description->cols = strlen(message);
    description->posx = 0;
    description->posy = 0;
  }

  description->number = currentVirtualTerminal_AtSpi2Screen();
}

static int
readCharacters_AtSpi2Screen (const ScreenBox *box, ScreenCharacter *buffer) {
  clearScreenCharacters(buffer, (box->height * box->width));

  if (!curPath) {
    setScreenMessage(box, buffer, nonatspi);
    return 1;
  }

  if (!curNumCols || !curNumRows) return 0;
  short cols = (curPosX >= curNumCols)? (curPosX + 1): curNumCols;
  if (!validateScreenBox(box, cols, curNumRows)) return 0;

  for (unsigned int y=0; y<box->height; y+=1) {
    if (curRowLengths[box->top+y]) {
      for (unsigned int x=0; x<box->width; x+=1) {
        if (box->left+x < curRowLengths[box->top+y] - (curRows[box->top+y][curRowLengths[box->top+y]-1]=='\n')) {
          buffer[y*box->width+x].text = curRows[box->top+y][box->left+x];
        }
      }
    }
  }

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
  DBusMessage *msg, *reply;
  char *s = "";

  msg = new_method_call(SPI2_DBUS_INTERFACE_REG, SPI2_DBUS_PATH_DEC, SPI2_DBUS_INTERFACE_DEC, "GenerateKeyboardEvent");
  if (!msg)
    return 0;
  dbus_message_append_args(msg, DBUS_TYPE_INT32, &keysym, DBUS_TYPE_STRING, &s, DBUS_TYPE_UINT32, &key_type, DBUS_TYPE_INVALID);
  reply = send_with_reply_and_block(bus, msg, 1000, "generating keyboard event");
  if (!reply)
    return 0;

  return 1;
}

static int
insertKey_AtSpi2Screen (ScreenKey key) {
  long keysym;
  int modMeta=0, modControl=0;

  setScreenKeyModifiers(&key, SCR_KEY_CONTROL);

  if (isSpecialKey(key)) {
    switch (key & SCR_KEY_CHAR_MASK) {
#ifdef HAVE_X11_KEYSYM_H
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
#else /* HAVE_X11_KEYSYM_H */
#warning insertion of non-character key presses not supported by this build - check that X11 protocol headers have been installed
#endif /* HAVE_X11_KEYSYM_H */
      default: logMessage(LOG_WARNING, "key not insertable: %04X", key); return 0;
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
  logMessage(LOG_CATEGORY(SCREEN_DRIVER),
             "inserting key: %04X -> %s%s%ld",
             key,
             (modMeta? "meta ": ""),
             (modControl? "control ": ""),
             keysym);

  {
    int ok = 0;

#ifdef HAVE_X11_KEYSYM_H
    if (!modMeta || AtSpi2GenerateKeyboardEvent(XK_Meta_L,PRESS)) {
      if (!modControl || AtSpi2GenerateKeyboardEvent(XK_Control_L,PRESS)) {
#endif /* HAVE_X11_KEYSYM_H */

        if (AtSpi2GenerateKeyboardEvent(keysym,SYM)) {
          ok = 1;
        } else {
          logMessage(LOG_WARNING, "key insertion failed.");
        }

#ifdef HAVE_X11_KEYSYM_H
        if (modControl && !AtSpi2GenerateKeyboardEvent(XK_Control_L,RELEASE)) {
          logMessage(LOG_WARNING, "control release failed.");
          ok = 0;
        }
      } else {
        logMessage(LOG_WARNING, "control press failed.");
      }

      if (modMeta && !AtSpi2GenerateKeyboardEvent(XK_Meta_L,RELEASE)) {
        logMessage(LOG_WARNING, "meta release failed.");
        ok = 0;
      }
    } else {
      logMessage(LOG_WARNING, "meta press failed.");
    }
#endif /* HAVE_X11_KEYSYM_H */

    return ok;
  }
}

static void
scr_initialize (MainScreen *main) {
  initializeRealScreen(main);
  main->base.poll = poll_AtSpi2Screen;
  main->base.refresh = refresh_AtSpi2Screen;
  main->base.describe = describe_AtSpi2Screen;
  main->base.readCharacters = readCharacters_AtSpi2Screen;
  main->base.insertKey = insertKey_AtSpi2Screen;
  main->base.selectVirtualTerminal = selectVirtualTerminal_AtSpi2Screen;
  main->base.switchVirtualTerminal = switchVirtualTerminal_AtSpi2Screen;
  main->base.currentVirtualTerminal = currentVirtualTerminal_AtSpi2Screen;
  main->processParameters = processParameters_AtSpi2Screen;
  main->construct = construct_AtSpi2Screen;
  main->destruct = destruct_AtSpi2Screen;
}
