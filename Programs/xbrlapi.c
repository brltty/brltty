/*
 * XBrlAPI - A background process tinkering with X for proper BrlAPI behavior
 *
 * Copyright (C) 2003-2008 by Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 *
 * XBrlAPI comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* Compile with:
 * gcc -O3 -Wall xbrlapi.c -L/usr/X11R6/lib -lbrlapi -lX11 -o xbrlapi
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <langinfo.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif /* HAVE_ICONV_H */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#undef CAN_SIMULATE_KEY_PRESSES
#if defined(HAVE_X11_EXTENSIONS_XTEST_H) && defined(HAVE_X11_EXTENSIONS_XKB_H)
#include <X11/extensions/XTest.h>
#define CAN_SIMULATE_KEY_PRESSES
#else /* HAVE_X11_EXTENSIONS_XTEST_H && HAVE_X11_EXTENSIONS_XKB_H */
#warning key press simulation not supported on this installation
#endif /* HAVE_X11_EXTENSIONS_XTEST_H && HAVE_X11_EXTENSIONS_XKB_H */

#define BRLAPI_NO_DEPRECATED
#include "brlapi.h"

#include "misc.h"
#include "options.h"

//#define DEBUG
#ifdef DEBUG
#define debugf(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#else /* DEBUG */
#define debugf(fmt, ...) (void)0
#endif /* DEBUG */

/******************************************************************************
 * option handling
 */

static char *auth;
static char *host;
static char *xDisplay;

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'b',
    .word = "brlapi",
    .argument = strtext("[host][:port]"),
    .setting.string = &host,
    .description = strtext("BrlAPI host and/or port to connect to")
  },

  { .letter = 'a',
    .word = "auth",
    .argument = strtext("file"),
    .setting.string = &auth,
    .description = strtext("BrlAPI authorization/authentication string")
  },

  { .letter = 'd',
    .word = "display",
    .argument = strtext("display"),
    .setting.string = &xDisplay,
    .description = strtext("X display to connect to")
  },
END_OPTION_TABLE

/******************************************************************************
 * error handling
 */

void fatal_brlapi_errno(const char *msg, const char *fmt, ...) {
  brlapi_perror(msg);
  if (fmt) {
    va_list va;
    va_start(va,fmt);
    vfprintf(stderr,fmt,va);
    va_end(va);
  }
  exit(1);
}

void fatal_errno(const char *msg, const char *fmt, ...) {
  perror(msg);
  if (fmt) {
    va_list va;
    va_start(va,fmt);
    vfprintf(stderr,fmt,va);
    va_end(va);
  }
  exit(1);
}

void fatal(const char *fmt, ...) {
  if (fmt) {
    va_list va;
    va_start(va,fmt);
    vfprintf(stderr,fmt,va);
    va_end(va);
  }
  exit(1);
}

/******************************************************************************
 * brlapi handling
 */


#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))
#endif /* MIN */

static int brlapi_fd;

void api_cleanExit(int foo) {
  close(brlapi_fd);
  exit(0);
}

void tobrltty_init(char *auth, char *host) {
  brlapi_connectionSettings_t settings;
  unsigned int x,y;
  settings.host=host;
  settings.auth=auth;

  signal(SIGTERM,api_cleanExit);
  signal(SIGINT,api_cleanExit);
#ifdef SIGHUP
  signal(SIGHUP,api_cleanExit);
#endif /* SIGHUP */
#ifdef SIGQUIT
  signal(SIGQUIT,api_cleanExit);
#endif /* SIGQUIT */
#ifdef SIGPIPE
  signal(SIGPIPE,api_cleanExit);
#endif /* SIGPIPE */

  if ((brlapi_fd = brlapi_openConnection(&settings,&settings))<0)
    fatal_brlapi_errno("openConnection",gettext("cannot connect to brltty at %s\n"),settings.host);

  if (brlapi_getDisplaySize(&x,&y)<0)
    fatal_brlapi_errno("getDisplaySize",NULL);
}

static int getXVTnb(void);

void getVT(void) {
  if (getenv("WINDOWPATH")) {
    if (brlapi_enterTtyModeWithPath(NULL,0,NULL)<0)
      fatal_brlapi_errno("geTtyPath",gettext("cannot get tty\n"));
  } else {
    int vtno = getXVTnb();
    if (brlapi_enterTtyMode(vtno,NULL)<0)
      fatal_brlapi_errno("enterTtyMode",gettext("cannot get tty %d\n"),vtno);
  }
  if (brlapi_ignoreAllKeys()<0)
    fatal_brlapi_errno("ignoreAllKeys",gettext("cannot ignore keys\n"));
#ifdef CAN_SIMULATE_KEY_PRESSES
  /* All X keysyms with any modifier */
  brlapi_keyCode_t cmd = BRLAPI_KEY_TYPE_SYM;
  if (brlapi_acceptKeys(brlapi_rangeType_type, &cmd, 1))
    fatal_brlapi_errno("acceptKeys",NULL);
#endif /* CAN_SIMULATE_KEY_PRESSES */
}

void api_setName(const char *wm_name) {
  static char *last_name;

  debugf("%s got focus\n",wm_name);
  if (last_name) {
    if (!strcmp(wm_name,last_name)) return;
    free(last_name);
  }
  if (!(last_name=strdup(wm_name))) fatal_errno("strdup(wm_name)",NULL);

  if (brlapi_writeText(0,wm_name)<0) {
    brlapi_perror("writeText");
    fprintf(stderr,gettext("cannot write window name %s\n"),wm_name);
  }
}

void api_setFocus(int win) {
  debugf("%#010x (%d) got focus\n",win,win);
  if (brlapi_setFocus(win)<0)
    fatal_brlapi_errno("setFocus",gettext("cannot set focus to %#010x\n"),win);
}

/******************************************************************************
 * X handling
 */

static const char *Xdisplay;
static Display *dpy;

static Window curWindow;
static Atom netWmNameAtom, utf8StringAtom;

static volatile int grabFailed;

#ifdef HAVE_ICONV_H
iconv_t utf8Conv = (iconv_t)(-1);
#endif /* HAVE_ICONV_H */

#define WINHASHBITS 12

static struct window {
  Window win;
  Window root;
  char *wm_name;
  struct window *next;
} *windows[(1<<WINHASHBITS)];

#define WINHASH(win) windows[(win)>>(32-WINHASHBITS)^(win&((1<<WINHASHBITS)-1))]

static void add_window(Window win, Window root, char *wm_name) {
  struct window *cur;
  if (!(cur=malloc(sizeof(struct window))))
    fatal_errno("malloc(struct window)",NULL);
  cur->win=win;
  cur->wm_name=wm_name;
  cur->root=root;
  cur->next=WINHASH(win);
  WINHASH(win)=cur;
}

static struct window *window_of_Window(Window win) {
  struct window *cur;
  for (cur=WINHASH(win); cur && cur->win!=win; cur=cur->next);
  return cur;
}

static int del_window(Window win) {
  struct window **pred;
  struct window *cur;
  
  for (pred=&WINHASH(win); cur = *pred, cur && cur->win!=win; pred=&cur->next);

  if (cur) {
    *pred=cur->next;
    free(cur->wm_name);
    free(cur);
    return 0;
  } else return -1;
}

static int ErrorHandler(Display *dpy, XErrorEvent *ev) {
  char buffer[128];
  if (ev->error_code==BadWindow) {
    grabFailed=1;
    return 0;
  }
  if (!XGetErrorText(dpy, ev->error_code, buffer, sizeof(buffer)))
    fatal("XGetErrorText");
  fprintf(stderr,gettext("X Error %d, %s on display %s\n"), ev->type, buffer, XDisplayName(Xdisplay));
  fprintf(stderr,gettext("resource %#010lx, req %u:%u\n"),ev->resourceid,ev->request_code,ev->minor_code);
  exit(1);
}

static int getXVTnb(void) {
  Window root;
  Atom property;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *buf;
  int vt = -1;

  root=DefaultRootWindow(dpy);

  if ((property=XInternAtom(dpy,"XFree86_VT",False))==None)
    fatal(gettext("no XFree86_VT atom\n"));
  
  if (XGetWindowProperty(dpy,root,property,0,1,False,AnyPropertyType,
    &actual_type, &actual_format, &nitems, &bytes_after, &buf))
    fatal(gettext("cannot get root window XFree86_VT property\n"));

  if (nitems<1)
    fatal(gettext("no items for VT number\n"));
  if (nitems>1)
    fprintf(stderr,gettext("more than one item for VT number\n"));
  switch (actual_type) {
  case XA_CARDINAL:
  case XA_INTEGER:
  case XA_WINDOW:
    switch (actual_format) {
    case 8:  vt = (*(uint8_t *)buf); break;
    case 16: vt = (*(uint16_t *)buf); break;
    case 32: vt = (*(uint32_t *)buf); break;
    default: fatal(gettext("bad format for VT number\n"));
    }
    break;
  default: fatal(gettext("bad type for VT number\n"));
  }
  if (!XFree(buf)) fatal("XFree(VTnobuf)");
  return vt;
}

static int grabWindow(Window win,int level) {
#ifdef DEBUG
  char spaces[level+1];
#endif

  grabFailed=0;
  if (!XSelectInput(dpy,win,PropertyChangeMask|FocusChangeMask|SubstructureNotifyMask) || grabFailed)
    return 0;

#ifdef DEBUG
  memset(spaces,' ',level);
  spaces[level]='\0';
  debugf("%sgrabbed %#010lx\n",spaces,win);
#endif
  return 1;
}

static char *getWindowTitle(Window win) {
  int wm_name_size=32;
  Atom actual_type;
  int actual_format;
  unsigned long nitems,bytes_after;
  unsigned char *wm_name=NULL;
  char *ret;

  do {
    if (XGetWindowProperty(dpy,win,netWmNameAtom,0,wm_name_size,False,
	/*XA_STRING*/AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,
	&wm_name)) {
      wm_name = NULL;
      break; /* window disappeared or not available */
    }
    wm_name_size+=bytes_after;
    if (!bytes_after) break;
    if (!XFree(wm_name)) fatal("tempo_XFree(wm_name)");
  } while (1);
  if (!wm_name) do {
    if (XGetWindowProperty(dpy,win,XA_WM_NAME,0,wm_name_size,False,
	/*XA_STRING*/AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,
	&wm_name))
      return NULL; /* window disappeared */
    if (wm_name_size >= nitems + 1) break;
    wm_name_size += bytes_after + 1;
    if (!XFree(wm_name)) fatal("tempo_XFree(wm_name)");
  } while (1);
  if (actual_type==None) {
    XFree(wm_name);
    return NULL;
  }
  wm_name[nitems++] = 0;
  ret = strdup((char *) wm_name);
  XFree(wm_name);
  debugf("type %lx name %s len %ld\n",actual_type,ret,nitems);
#ifdef HAVE_ICONV_H
  {
    if (actual_type == utf8StringAtom && utf8Conv != (iconv_t)(-1)) {
      char *ret2;
      size_t input_size, output_size;
      char *input, *output;

      input_size = nitems;
      input = ret;
      output_size = nitems * MB_CUR_MAX;
      output = ret2 = malloc(output_size);
      if (iconv(utf8Conv, &input, &input_size, &output, &output_size) == -1) {
	free(ret2);
      } else {
	free(ret);
	ret = realloc(ret2, nitems * MB_CUR_MAX - output_size);
	debugf("-> %s\n",ret);
      }
    }
  }
#endif /* HAVE_ICONV_H */
  return ret;
}

static int grabWindows(Window win,int level) {
  Window root,parent,*children;
  unsigned int nchildren,i;
  int res=1;

  if (!grabWindow(win,level)) return 1; /* window disappeared */

  if (!XQueryTree(dpy,win,&root,&parent,&children,&nchildren)) return 0;

  add_window(win,root,getWindowTitle(win));

  if (!children) return 1;

  for (i=0;i<nchildren;i++)
    if (children[i] && !grabWindows(children[i],level+1)) {
      res=0;
      break;
    }

  if (!XFree(children)) fatal("XFree(children)");
  return res;
}

static void setName(const struct window *window) {
  if (!window->wm_name)
    if (window->win==window->root) api_setName("root");
    else api_setName("unknown");
  else api_setName(window->wm_name);
}

static void setFocus(Window win) {
  struct window *window;

  curWindow=win;
  api_setFocus((uint32_t)win);

  if (!(window=window_of_Window(win))) {
    fprintf(stderr,gettext("didn't grab window %#010lx but got focus\n"),win);
    api_setName("unknown");
  } else setName(window);
}

#ifdef CAN_SIMULATE_KEY_PRESSES
static int tryModifiers(KeyCode keycode, unsigned int *modifiers, unsigned int modifiers_try, KeySym keysym) {
  KeySym keysymRet;
  unsigned int modifiersRet;
  if (!XkbLookupKeySym(dpy, keycode, *modifiers | modifiers_try, &modifiersRet, &keysymRet))
    return 0;
  if (keysymRet != keysym)
    return 0;
  *modifiers |= modifiers_try;
  return 1;
}
#endif /* CAN_SIMULATE_KEY_PRESSES */

void toX_f(const char *display) {
  Window root;
  XEvent ev;
  int i;
  int X_fd;
  fd_set fds,readfds;
  int maxfd;
#ifdef CAN_SIMULATE_KEY_PRESSES
  int res;
  brlapi_keyCode_t code;
  unsigned int keysym, keycode, modifiers;
  Bool haveXTest;
  int eventBase, errorBase, majorVersion, minorVersion;
#endif /* CAN_SIMULATE_KEY_PRESSES */

  Xdisplay = display;
  if (!Xdisplay) Xdisplay=getenv("DISPLAY");
  if (!(dpy=XOpenDisplay(Xdisplay))) fatal(gettext("cannot connect to display %s\n"),Xdisplay);

  if (!XSetErrorHandler(ErrorHandler)) fatal(gettext("strange old error handler\n"));

#ifdef CAN_SIMULATE_KEY_PRESSES
  haveXTest = XTestQueryExtension(dpy, &eventBase, &errorBase, &majorVersion, &minorVersion);
#endif /* CAN_SIMULATE_KEY_PRESSES */

  {
    int foo;
    int major = XkbMajorVersion, minor = XkbMinorVersion;
    if (!XkbLibraryVersion(&major, &minor))
      fatal(gettext("Incompatible XKB library\n"));
    if (!XkbQueryExtension(dpy, &foo, &foo, &foo, &major, &minor))
      fatal(gettext("Incompatible XKB server support\n"));
  }

  X_fd = XConnectionNumber(dpy);
  FD_ZERO(&fds);
  FD_SET(brlapi_fd, &fds);
  FD_SET(X_fd, &fds);
  maxfd = X_fd>brlapi_fd ? X_fd+1 : brlapi_fd+1;

  getVT();
  netWmNameAtom = XInternAtom(dpy,"_NET_WM_NAME",False);
  utf8StringAtom = XInternAtom(dpy,"UTF8_STRING",False);

  {
    char *localCharset = nl_langinfo(CODESET);
    if (strcmp(localCharset, "UTF-8")) {
      char buf[strlen(localCharset) + 10 + 1];
      snprintf(buf, sizeof(buf), "%s//TRANSLIT", localCharset);
      if ((utf8Conv = iconv_open(buf, "UTF-8")) == (iconv_t)(-1))
        utf8Conv = iconv_open(localCharset, "UTF-8");
    }
  }

  for (i=0;i<ScreenCount(dpy);i++) {
    root=RootWindow(dpy,i);
    if (!grabWindows(root,0)) fatal(gettext("cannot grab windows on screen %d\n"),i);
  }

  {
    Window win;
    int revert_to;
    if (!XGetInputFocus(dpy,&win,&revert_to))
      fatal(gettext("failed to get first focus\n"));
    setFocus(win);
  }
  while(1) {
    XFlush(dpy);
    memcpy(&readfds,&fds,sizeof(readfds));
    if (select(maxfd,&readfds,NULL,NULL,NULL)<0)
      fatal_errno("select",NULL);
    if (FD_ISSET(X_fd,&readfds))
    while (XPending(dpy)) {
      if ((i=XNextEvent(dpy,&ev)))
	fatal("XNextEvent: %d\n",i);
      switch (ev.type) {

      /* focus events */
      case FocusIn:
	switch (ev.xfocus.detail) {
	case NotifyAncestor:
	case NotifyInferior:
	case NotifyNonlinear:
	case NotifyPointerRoot:
	case NotifyDetailNone:
	  setFocus(ev.xfocus.window); break;
	} break;
      case FocusOut:
	/* ignore
	switch (ev.xfocus.detail) {
	case NotifyAncestor:
	case NotifyInferior:
	case NotifyNonlinear:
	case NotifyPointerRoot:
	case NotifyDetailNone:
	printf("win %#010lx lost focus\n",ev.xfocus.window);
	break;
	}
	*/
	break;

      /* create & destroy events */
      case CreateNotify: {
      /* there's a race condition here : a window may get the focus or change
       * its title between it is created and we achieve XSelectInput on it */
	Window win = ev.xcreatewindow.window;
	struct window *window;
	if (!grabWindow(win,0)) break; /* window already disappeared ! */
	debugf("win %#010lx created\n",win);
	if (!(window = window_of_Window(ev.xcreatewindow.parent))) {
	  fprintf(stderr,gettext("didn't grab parent of %#010lx\n"),win);
	  add_window(win,None,getWindowTitle(win));
	} else add_window(win,window->root,getWindowTitle(win));
      } break;
      case DestroyNotify:
	debugf("win %#010lx destroyed\n",ev.xdestroywindow.window);
	if (del_window(ev.xdestroywindow.window))
	  debugf("destroy: didn't grab window %#010lx\n",ev.xdestroywindow.window);
	break;

      /* Property change: WM_NAME ? */
      case PropertyNotify:
	if (ev.xproperty.atom==XA_WM_NAME ||
	    (netWmNameAtom != None && ev.xproperty.atom == netWmNameAtom)) {
	  Window win = ev.xproperty.window;
	  debugf("WM_NAME property of %#010lx changed\n",win);
	  struct window *window;
	  if (!(window=window_of_Window(win))) {
	    fprintf(stderr,gettext("didn't grab window %#010lx\n"),win);
	    add_window(win,None,getWindowTitle(win));
	  } else {
	    if (window->wm_name)
	      if (!XFree(window->wm_name)) fatal(gettext("XFree(wm_name) for change"));
	    if ((window->wm_name=getWindowTitle(win))) {
	      if (win==curWindow)
		api_setName(window->wm_name);
	    } else fprintf(stderr,gettext("window %#010lx changed to NULL name\n"),win);
	  }
	}
	break;
      case MappingNotify:
	XRefreshKeyboardMapping(&ev.xmapping);
	break;
      /* ignored events */
      case UnmapNotify:
      case MapNotify:
      case MapRequest:
      case ReparentNotify:
      case ConfigureNotify:
      case GravityNotify:
      case ConfigureRequest:
      case CirculateNotify:
      case CirculateRequest:
	break;

      /* "shouldn't happen" events */
      default: fprintf(stderr,gettext("unhandled event type: %d\n"),ev.type); break;
      }
    }
#ifdef CAN_SIMULATE_KEY_PRESSES
    if (haveXTest && FD_ISSET(brlapi_fd,&readfds)) {
      while ((res = brlapi_readKey(0, &code)==1)) {
	if (((code & BRLAPI_KEY_TYPE_MASK) != BRLAPI_KEY_TYPE_SYM)) {
	  fprintf(stderr,gettext("unexpected block type %" BRLAPI_PRIxKEYCODE "\n"),code);
	  continue;
	}

	modifiers = ((code & BRLAPI_KEY_FLAGS_MASK) >> BRLAPI_KEY_FLAGS_SHIFT) & 0xFF;
	keysym = code & BRLAPI_KEY_CODE_MASK;
	keycode = XKeysymToKeycode(dpy,keysym);
	if (keycode == NoSymbol) {
	  fprintf(stderr,gettext("Couldn't translate keysym %08X to keycode.\n"),keysym);
	  continue;
	}

        {
          static const unsigned int tryTable[] = {
            0,
            ShiftMask,
            Mod2Mask,
            Mod3Mask,
            Mod4Mask,
            Mod5Mask,
            ShiftMask|Mod2Mask,
            ShiftMask|Mod3Mask,
            ShiftMask|Mod4Mask,
            ShiftMask|Mod5Mask,
            0
          };
          const unsigned int *try = tryTable;

          do {
            if (tryModifiers(keycode, &modifiers, *try, keysym)) goto foundModifiers;
          } while (*++try);

	  fprintf(stderr,gettext("Couldn't find modifiers to apply to %d for getting keysym %08X\n"),keycode,keysym);
	  continue;
        }
      foundModifiers:

	debugf("key %08X: (%d,%x)\n", keysym, keycode, modifiers);
	if (modifiers)
	  XkbLockModifiers(dpy, XkbUseCoreKbd, modifiers, modifiers);
	XTestFakeKeyEvent(dpy,keycode,True,CurrentTime);
	XTestFakeKeyEvent(dpy,keycode,False,CurrentTime);
	if (modifiers)
	  XkbLockModifiers(dpy, XkbUseCoreKbd, modifiers, 0);
      }
      if (res<0)
	fatal_brlapi_errno("brlapi_readKey",NULL);
    }
#endif /* CAN_SIMULATE_KEY_PRESSES */
  }
}

/******************************************************************************
 * main 
 */

int main(int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "xbrlapi"
    };
    processOptions(&descriptor, &argc, &argv);
  }

  tobrltty_init(auth,host);

  toX_f(xDisplay);

  return 0;
}
