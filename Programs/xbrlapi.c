/*
 * XBrlAPI - A background process tinkering with X for proper BrlAPI behavior
 *
 * Copyright (C) 2003-2005 by Samuel Thibault <Samuel.Thibault@ens-lyon.org>
 * All rights reserved.
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#ifdef WINDOWS
#include <ws2tcpip.h>
#else /* WINDOWS */
#include <netinet/in.h>
#endif /* WINDOWS */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else /* HAVE_SYS_SELECT_H */
#include <sys/time.h>
#endif /* HAVE_SYS_SELECT_H */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef HAVE_X11_EXTENSIONS_XTEST_H
#include <X11/extensions/XTest.h>
#else /* HAVE_X11_EXTENSIONS_XTEST_H */
#warning <X11/extensions/XTest.h> not available: keypress simulation not supported
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include "api.h"
#include "brldefs.h"

//#define DEBUG
#ifdef DEBUG
#define debugf(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#else
#define debugf(fmt, ...) (void)0
#endif

#define _(s) (s)
#define _n(s,sp,n) ((n)>1?(sp):(s))
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
  va_list va;
  va_start(va,fmt);
  if (fmt)
    vfprintf(stderr,fmt,va);
  exit(1);
  va_end(va);
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

void tobrltty_init(char *authKey, char *hostName) {
  brlapi_settings_t settings;
  unsigned int x,y;
  settings.hostName=hostName;
  settings.authKey=authKey;

  signal(SIGTERM,api_cleanExit);
  signal(SIGHUP,api_cleanExit);
  signal(SIGINT,api_cleanExit);
  signal(SIGQUIT,api_cleanExit);
  signal(SIGPIPE,api_cleanExit);

  if ((brlapi_fd = brlapi_initializeConnection(&settings,&settings))<0)
    fatal_brlapi_errno("initializeConnection",_("couldn't connect to brltty at %s\n"),settings.hostName);

  if (brlapi_getDisplaySize(&x,&y)<0)
    fatal_brlapi_errno("getDisplaySize",NULL);
}

void vtno(int vtno) {
  if (brlapi_getTty(vtno,NULL)<0)
    fatal_brlapi_errno("getTty",_("getting tty %d\n"),vtno);
  if (brlapi_ignoreKeyRange(0,BRL_KEYCODE_MAX)<0)
    fatal_brlapi_errno("ignoreKeys",_("ignoring every key %d\n"),vtno);
#ifdef HAVE_X11_EXTENSIONS_XTEST_H
  if (brlapi_unignoreKeyRange(BRL_BLK_PASSCHAR,BRL_BLK_PASSCHAR|BRL_MSK_ARG))
    fatal_brlapi_errno("unignoreKeyRange",NULL);
  if (brlapi_unignoreKeyRange(BRL_BLK_PASSKEY, BRL_BLK_PASSKEY |BRL_MSK_ARG))
    fatal_brlapi_errno("unignoreKeyRange",NULL);
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */
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
    fprintf(stderr,_("setting name %s\n"),wm_name);
  }
}

void api_setFocus(int win) {
  debugf("%#010x (%d) got focus\n",win,win);
  if (brlapi_setFocus(win)<0)
    fatal_brlapi_errno("setFocus",_("setting focus to %#010x\n"),win);
}

/******************************************************************************
 * X handling
 */

static const char *Xdisplay;
static Display *dpy;

static Window curWindow;

static volatile int grabFailed;

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
    if (cur->wm_name && !XFree(cur->wm_name)) fatal("XFree(wm_name)");
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
  fprintf(stderr,_("X Error %d, %s on display %s\n"), ev->type, buffer, XDisplayName(Xdisplay));
  fprintf(stderr,_("resource %#010lx, req %u:%u\n"),ev->resourceid,ev->request_code,ev->minor_code);
  exit(1);
}

static void getVTnb(void) {
  Window root;
  Atom property;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *buf;

  root=DefaultRootWindow(dpy);

  if ((property=XInternAtom(dpy,"XFree86_VT",False))==None)
    fatal(_("no XFree86_VT atom\n"));
  
  if (XGetWindowProperty(dpy,root,property,0,1,False,AnyPropertyType,
    &actual_type, &actual_format, &nitems, &bytes_after, &buf))
    fatal(_("couldn't get root window XFree86_VT property\n"));

  if (nitems<1)
    fatal(_("no items for VT number\n"));
  if (nitems>1)
    fprintf(stderr,_("several items for VT number ?!\n"));
  switch (actual_type) {
  case XA_CARDINAL:
  case XA_INTEGER:
  case XA_WINDOW:
    switch (actual_format) {
    case 8:  vtno(*(uint8_t *)buf); break;
    case 16: vtno(*(uint16_t *)buf); break;
    case 32: vtno(*(uint32_t *)buf); break;
    default: fatal(_("Bad format for VT number\n"));
    }
    break;
  default: fatal(_("Bad type for VT number\n"));
  }
  if (!XFree(buf)) fatal("XFree(VTnobuf)");
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

  do {
    if (XGetWindowProperty(dpy,win,XA_WM_NAME,0,wm_name_size,False,/*XA_STRING*/AnyPropertyType,
      &actual_type,&actual_format,&nitems,&bytes_after,&wm_name))
      return NULL; /* window disappeared */
    wm_name_size+=bytes_after;
  } while (bytes_after && ({if (!XFree(wm_name)) fatal("tempo_XFree(wm_name)"); 1;}));
  if (actual_type==None) return NULL;
  else {
    debugf("type %lx name %s",actual_type,wm_name);
    return (char *) wm_name;
  }
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
    fprintf(stderr,_("didn't grab window %#010lx but got focus !\n"),win);
    api_setName("unknown");
  } else setName(window);
}

void toX_f(const char *display) {
  Window root;
  XEvent ev;
  int i;
  int X_fd;
  fd_set fds,readfds;
  int maxfd;
#ifdef HAVE_X11_EXTENSIONS_XTEST_H
  int res;
  brl_keycode_t code;
  unsigned int keysym, keycode, modifiers;
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */

  Xdisplay = display;
  if (!Xdisplay) Xdisplay=getenv("DISPLAY");
  if (!(dpy=XOpenDisplay(Xdisplay))) fatal(_("couldn't connect to %s\n"),Xdisplay);

  if (!XSetErrorHandler(ErrorHandler)) fatal(_("strange old error handler\n"));

  X_fd = XConnectionNumber(dpy);
  FD_ZERO(&fds);
  FD_SET(brlapi_fd, &fds);
  FD_SET(X_fd, &fds);
  maxfd = X_fd>brlapi_fd ? X_fd+1 : brlapi_fd+1;

  getVTnb();
  
  for (i=0;i<ScreenCount(dpy);i++) {
    root=RootWindow(dpy,i);
    if (!grabWindows(root,0)) fatal(_("Couldn't grab windows on screen %d\n"),i);
  }

  {
    Window win;
    int revert_to;
    if (!XGetInputFocus(dpy,&win,&revert_to))
      fatal(_("failed to get first Focus\n"));
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
	  fprintf(stderr,_("didn't grab parent of %#010lx ?!\n"),win);
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
	if (ev.xproperty.atom==XA_WM_NAME) {
	  Window win = ev.xproperty.window;
	  debugf("WM_NAME property of %#010lx changed\n",win);
	  struct window *window;
	  if (!(window=window_of_Window(win))) {
	    fprintf(stderr,_("didn't grab window %#010lx\n"),win);
	    add_window(win,None,getWindowTitle(win));
	  } else {
	    if (window->wm_name)
	      if (!XFree(window->wm_name)) fatal(_("XFree(wm_name) for change"));
	    if ((window->wm_name=getWindowTitle(win))) {
	      if (win==curWindow)
		api_setName(window->wm_name);
	    } else fprintf(stderr,_("uhu, window %#010lx changed to NULL name ?!\n"),win);
	  }
	}
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
      default: fprintf(stderr,_("unhandled %d type\n"),ev.type); break;
      }
    }
#ifdef HAVE_X11_EXTENSIONS_XTEST_H
    if (FD_ISSET(brlapi_fd,&readfds)) {
      while ((res = brlapi_readKey(0, &code)==1)) {
	modifiers = 0;
	if (code & BRL_FLG_CHAR_CONTROL) modifiers |= ControlMask;
	if (code & BRL_FLG_CHAR_META) modifiers |= Mod1Mask;
	if (code & BRL_FLG_CHAR_UPPER) modifiers |= ShiftMask;
	if (code & BRL_FLG_CHAR_SHIFT) modifiers |= ShiftMask;

	switch (code&BRL_MSK_BLK) {
	  case BRL_BLK_PASSCHAR:
	    if (((code&BRL_MSK_ARG) >= 'A') && ((code&BRL_MSK_ARG) <= 'Z')) {
	      code = code + 'a' - 'A';
	      modifiers |= ShiftMask;
	    }
	    keysym = code&BRL_MSK_ARG;
	    break;

	  case BRL_BLK_PASSKEY:
	    switch (code&BRL_MSK_ARG) {
	      case BRL_KEY_ENTER:         keysym = XK_KP_Enter;  break;
	      case BRL_KEY_TAB:           keysym = XK_Tab;       break;
	      case BRL_KEY_BACKSPACE:     keysym = XK_BackSpace; break;
	      case BRL_KEY_ESCAPE:        keysym = XK_Escape;    break;
	      case BRL_KEY_CURSOR_LEFT:   keysym = XK_Left;      break;
	      case BRL_KEY_CURSOR_RIGHT:  keysym = XK_Right;     break;
	      case BRL_KEY_CURSOR_UP:     keysym = XK_Up;        break;
	      case BRL_KEY_CURSOR_DOWN:   keysym = XK_Down;      break;
	      case BRL_KEY_PAGE_UP:       keysym = XK_Page_Up;   break;
	      case BRL_KEY_PAGE_DOWN:     keysym = XK_Page_Down; break;
	      case BRL_KEY_HOME:          keysym = XK_Home;      break;
	      case BRL_KEY_END:           keysym = XK_End;       break;
	      case BRL_KEY_INSERT:        keysym = XK_Insert;    break;
	      case BRL_KEY_DELETE:        keysym = XK_Delete;    break;
	      case BRL_KEY_FUNCTION + 0:  keysym = XK_F1;        break;
	      case BRL_KEY_FUNCTION + 1:  keysym = XK_F2;        break;
	      case BRL_KEY_FUNCTION + 2:  keysym = XK_F3;        break;
	      case BRL_KEY_FUNCTION + 3:  keysym = XK_F4;        break;
	      case BRL_KEY_FUNCTION + 4:  keysym = XK_F5;        break;
	      case BRL_KEY_FUNCTION + 5:  keysym = XK_F6;        break;
	      case BRL_KEY_FUNCTION + 6:  keysym = XK_F7;        break;
	      case BRL_KEY_FUNCTION + 7:  keysym = XK_F8;        break;
	      case BRL_KEY_FUNCTION + 8:  keysym = XK_F9;        break;
	      case BRL_KEY_FUNCTION + 9:  keysym = XK_F10;       break;
	      case BRL_KEY_FUNCTION + 10: keysym = XK_F11;       break;
	      case BRL_KEY_FUNCTION + 11: keysym = XK_F12;       break;
	      case BRL_KEY_FUNCTION + 12: keysym = XK_F13;       break;
	      case BRL_KEY_FUNCTION + 13: keysym = XK_F14;       break;
	      case BRL_KEY_FUNCTION + 14: keysym = XK_F15;       break;
	      case BRL_KEY_FUNCTION + 15: keysym = XK_F16;       break;
	      case BRL_KEY_FUNCTION + 16: keysym = XK_F17;       break;
	      case BRL_KEY_FUNCTION + 17: keysym = XK_F18;       break;
	      case BRL_KEY_FUNCTION + 18: keysym = XK_F19;       break;
	      case BRL_KEY_FUNCTION + 19: keysym = XK_F20;       break;
	      case BRL_KEY_FUNCTION + 20: keysym = XK_F21;       break;
	      case BRL_KEY_FUNCTION + 21: keysym = XK_F22;       break;
	      case BRL_KEY_FUNCTION + 22: keysym = XK_F23;       break;
	      case BRL_KEY_FUNCTION + 23: keysym = XK_F24;       break;
	      case BRL_KEY_FUNCTION + 24: keysym = XK_F25;       break;
	      case BRL_KEY_FUNCTION + 25: keysym = XK_F26;       break;
	      case BRL_KEY_FUNCTION + 26: keysym = XK_F27;       break;
	      case BRL_KEY_FUNCTION + 27: keysym = XK_F28;       break;
	      case BRL_KEY_FUNCTION + 28: keysym = XK_F29;       break;
	      case BRL_KEY_FUNCTION + 29: keysym = XK_F30;       break;
	      case BRL_KEY_FUNCTION + 30: keysym = XK_F31;       break;
	      case BRL_KEY_FUNCTION + 31: keysym = XK_F32;       break;
	      case BRL_KEY_FUNCTION + 32: keysym = XK_F33;       break;
	      case BRL_KEY_FUNCTION + 33: keysym = XK_F34;       break;
	      case BRL_KEY_FUNCTION + 34: keysym = XK_F35;       break;
	      default:
		fprintf(stderr,"unexpected key: %" PRIX32 "\n",code);
		continue;
	    }
	  default:
	    fprintf(stderr,"unexpected block type %" PRIX32 "\n",code&BRL_MSK_BLK);
	    continue;
	}
	keycode = XKeysymToKeycode(dpy,keysym);
	if (modifiers)
	  XkbLockModifiers(dpy, XkbUseCoreKbd, modifiers, modifiers);
	debugf("key %x\n",code);
	XTestFakeKeyEvent(dpy,keycode,True,CurrentTime);
	XTestFakeKeyEvent(dpy,keycode,False,CurrentTime);
	if (modifiers)
	  XkbLockModifiers(dpy, XkbUseCoreKbd, modifiers, 0);
      }
      if (res<0)
	fatal_brlapi_errno("brlapi_readKey",NULL);
    }
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */
  }
}

/******************************************************************************
 * main 
 */

static char *keypath=NULL,*hostname=NULL;

static struct option longopts[] = {
  { "help",	0, NULL, 'h' },
  { "keypath",  1, NULL, 'k' },
  { "hostname", 1, NULL, 'H' },
  { "display",  1, NULL, 'd' },
  { NULL,       0, NULL,  0  },
};

static void usage(const char *name) {
  printf(_("usage: %s [opts...]\n"),name);
  printf(_("  --help                   get what you're reading\n"));
  printf(_("  --keypath path           set path to authentication key\n"));
  printf(_("  --hostname [host][:port] set local hostname and port to connect to\n"));
  printf(_("  --display display        set display\n"));
}

int main(int argc, char *argv[]) {
  int c;
  const char *display = NULL;

  while ((c=getopt_long(argc,argv,":h:H:k:d:f",longopts,NULL))!=EOF)
  switch (c) {
  case 'h':
    usage(argv[0]);
    exit(0);
  case 'k':
    keypath=optarg;
    break;
  case 'H':
    hostname=optarg;
    break;
  case 'd':
    display=optarg;
    break;
  case ':':
    fprintf(stderr,_("missing argument to -%c\n"),optopt);
    usage(argv[0]);
    exit(1);
  case '?':
    fprintf(stderr,_("bad option %c\n"),optopt);
    usage(argv[0]);
    exit(1);
  }

  tobrltty_init(keypath,hostname);

  toX_f(display);

  return 0;
}
