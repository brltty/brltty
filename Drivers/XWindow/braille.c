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
#define XW_VERSION "BRLTTY driver for X, version 0.1, 2004"
#define XW_COPYRIGHT "Copyright Samuel Thibault <samuel.thibault@ens-lyon.org>"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

#include "Programs/misc.h"

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#if defined(HAVE_PKG_XAW)
#define USE_XAW
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Repeater.h>
#elif defined(HAVE_PKG_XAW3D)
#define USE_XAW
#include <X11/Xaw3d/Form.h>
#include <X11/Xaw3d/Paned.h>
#include <X11/Xaw3d/Label.h>
#include <X11/Xaw3d/Command.h>
#include <X11/Xaw3d/Repeater.h>
#elif defined(HAVE_PKG_NEXTAW)
#define USE_XAW
#include <X11/neXtaw/Form.h>
#include <X11/neXtaw/Paned.h>
#include <X11/neXtaw/Label.h>
#include <X11/neXtaw/Command.h>
#include <X11/neXtaw/Repeater.h>
#elif defined(HAVE_PKG_XAWPLUS)
#define USE_XAW
#include <X11/XawPlus/Form.h>
#include <X11/XawPlus/Paned.h>
#include <X11/XawPlus/Label.h>
#include <X11/XawPlus/Command.h>
#include <X11/XawPlus/Repeater.h>
#elif defined(HAVE_PKG_XM)
#define USE_XM
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/PanedW.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#else /* HAVE_PKG_ */
#error Xt toolkit either unspecified or unsupported
#endif /* HAVE_PKG_ */

#if defined(USE_XAW)
#define formWidgetClass     formWidgetClass
#define panedWidgetClass    panedWidgetClass
#define labelWidgetClass    labelWidgetClass
#define commandWidgetClass  commandWidgetClass
#define repeaterWidgetClass repeaterWidgetClass
#define Nlabel              XtNlabel
#define Ncallback           XtNcallback
#define Ntop                XtNtop
#define Nbottom             XtNbottom
#define Nleft               XtNleft
#define Nright              XtNright
#define ChainTop            XtChainTop
#define ChainBottom         XtChainTop
#define ChainLeft           XtChainLeft
#define ChainRight          XtChainLeft
#define NvertDistance       XtNvertDistance
#define NhorizDistance      XtNhorizDistance
#elif defined(USE_XM)
#define formWidgetClass     xmFormWidgetClass
#define panedWidgetClass    xmPanedWindowWidgetClass
#define labelWidgetClass    xmLabelWidgetClass
#define commandWidgetClass  xmPushButtonWidgetClass
#define repeaterWidgetClass xmPushButtonWidgetClass
#define Nlabel              XmNlabelString
#define Ncallback           XmNactivateCallback
#define Ntop                XmNtopAttachment
#define Nbottom             XmNbottomAttachment
#define Nleft               XmNleftAttachment
#define Nright              XmNrightAttachment
#define ChainTop            XmATTACH_FORM
#define ChainBottom         XmATTACH_NONE
#define ChainLeft           XmATTACH_FORM
#define ChainRight          XmATTACH_NONE
#define NvertDistance       XmNtopOffset
#define NhorizDistance      XmNleftOffset
#else /* USE_ */
#error Xt paradigm either unspecified or unsupported
#endif /* USE_ */

typedef enum {
  PARM_XTPARMS,
  PARM_LINES,
  PARM_COLS,
  PARM_MODEL
} DriverParameter;
#define BRLPARMS "xtparms", "lines", "cols", "model"

#define BRL_HAVE_VISUAL_DISPLAY
#include "Programs/brl_driver.h"
#include "braille.h"

#define MAXLINES 3
#define MAXCOLS 80
#define WHOLESIZE (MAXLINES * MAXCOLS)
static int cols,lines;
static char *model;

#define BUTWIDTH 48
#define BUTHEIGHT 32

static Widget toplevel,vbox,hbox,keybox,display[WHOLESIZE];
#ifdef USE_XAW
static Widget displayb[WHOLESIZE];
static XFontSet fontset;
#endif
static XtAppContext app_con;
#ifdef USE_XM
static XmString display_cs;
#endif

static long keypressed;

static void KeyPressCB(Widget w, XtPointer closure, XtPointer callData) {
 LogPrint(LOG_DEBUG,"keypress(%p)",closure);
 keypressed=(long)closure;
}

static void route(Widget w, XEvent *event, String *params, Cardinal *num_params) {
 int index = atoi(params[0]);
 LogPrint(LOG_DEBUG,"route(%u)", index);
 keypressed = BRL_FLG_ROUTE | (index&BRL_MSK_ARG);
}

static inline Widget crKeyBut(String name, long keycode, int repeat,
    int horizDistance, int vertDistance)
{
  Widget button;
  button = XtVaCreateManagedWidget(name,
    repeat?repeaterWidgetClass:commandWidgetClass, keybox,
    XtNwidth, BUTWIDTH, XtNheight, BUTHEIGHT,
#ifdef USE_XAW
    XtNinitialDelay, 500, XtNminimumDelay, 100,
#endif
    NhorizDistance, horizDistance,
    NvertDistance, vertDistance,
    Ntop, ChainTop,
    Nbottom, ChainBottom,
    Nleft, ChainLeft,
    Nright, ChainRight,
    NULL);
  XtAddCallback(button, Ncallback, KeyPressCB, (XtPointer) keycode);
  return button;
}

struct button{
 String label;
 long keycode;
 int repeat;
 int x,y;
};

static struct button buttons_simple[] = {
 { "^",     BRL_CMD_LNUP,   1, 1, 0 },
 { "<",   BRL_CMD_FWINLT, 1, 0, 1 },
 { "Home",   BRL_CMD_HOME,   0, 1, 1 },
 { ">",  BRL_CMD_FWINRT, 1, 2, 1 },
 { "v",   BRL_CMD_LNDN,   1, 1, 2 },
 { "alt-a",  BRL_FLG_CHAR_META    | BRL_BLK_PASSCHAR | 'a', 0, 0, 3 },
 { "ctrl-a", BRL_FLG_CHAR_CONTROL | BRL_BLK_PASSCHAR | 'a', 0, 1, 3 },
 { "a",      BRL_BLK_PASSCHAR                        | 'a', 0, 2, 3 },
 { "A",      BRL_BLK_PASSCHAR                        | 'A', 0, 3, 3 },
 { NULL,     0,              0, 0, 0},
};

static struct button buttons_vs[] = {
	/*
 { "VT1", BRL_BLK_SWITCHVT,   1, 0, 1 },
 { "VT2", BRL_BLK_SWITCHVT+1, 1, 1, 1 },
 { "VT3", BRL_BLK_SWITCHVT+2, 1, 2, 1 },
 { "VT4", BRL_BLK_SWITCHVT+3, 1, 6, 1 },
 { "VT5", BRL_BLK_SWITCHVT+4, 1, 7, 1 },
 { "VT6", BRL_BLK_SWITCHVT+5, 1, 8, 1 },
	*/
 //{ "B5", EOF, /* cut */      1, 5, 2 },
 { "TOP", BRL_CMD_TOP_LEFT,   1, 6, 2 },
 { "BOT", BRL_CMD_BOT_LEFT,   1, 6, 4 },
 { "<=", BRL_CMD_FWINLTSKIP, 1, 1, 0 },
 { "<=", BRL_CMD_FWINLTSKIP, 1, 8, 2 },
 { "=>", BRL_CMD_FWINRTSKIP, 1, 2, 0 },
 { "=>", BRL_CMD_FWINRTSKIP, 1, 8, 4 },
 { "-^-",  BRL_CMD_LNUP,       1, 7, 2 },
 { "-v-",  BRL_CMD_LNDN,       1, 7, 4 },
 { "->", BRL_CMD_FWINRT,     1, 8, 3 },
 { "<-", BRL_CMD_FWINLT,     1, 6, 3 },
 { "HOME", BRL_CMD_HOME,       1, 7, 3 },
 { "^", BRL_BLK_PASSKEY + BRL_KEY_CURSOR_UP,    1, 1, 2 },
 { "v", BRL_BLK_PASSKEY + BRL_KEY_CURSOR_DOWN,  1, 1, 4 },
 { ">", BRL_BLK_PASSKEY + BRL_KEY_CURSOR_RIGHT, 1, 2, 3 },
 { "<", BRL_BLK_PASSKEY + BRL_KEY_CURSOR_LEFT,  1, 0, 3 },
 //{ "B3", BRL_CMD_CSRVIS,     1, 2, 2 },
 { "DEL", BRL_BLK_PASSKEY + BRL_KEY_DELETE,       1, 0, 4 },
 { "INS", BRL_BLK_PASSKEY + BRL_KEY_INSERT,       1, 2, 4 },
 //{ "C5", BRL_CMD_PASTE,      1, 5, 3 },
 //{ "D5", EOF,                1, 5, 4 },
 //{ "B4", EOF,                1, 3, 2 },

 //{ "B1", EOF,                1, 0, 2 },
 //{ "C2", EOF,                1, 1, 3 },
 //{ "C4", EOF,                1, 3, 3 },
 //{ "D4", EOF,                1, 3, 4 },
 { NULL,     0,              0, 0, 0},
};

static void createKeyButtons(struct button *buttons) {
 struct button *b;
 for (b=buttons; b->label; b++)
  crKeyBut(b->label, b->keycode, b->repeat, b->x*(BUTWIDTH+1), b->y*(BUTHEIGHT+1));
}

static int brl_readCommand(BrailleDisplay *brl, BRL_DriverCommandContext context)
{
 int res=EOF;
 while (XtAppPending(app_con)) {
  XtAppProcessEvent(app_con,XtIMAll);
  if (keypressed) {
   res=keypressed;
   keypressed=BRL_CMD_NOOP;
   break;
  }
 }
 return res;
}

static XrmOptionDescRec optionDescList[] = { };

static char *fallback_resources[] = {
 "*display.font: -*-fixed-*-*-*-*-24-*-*-*-*-*-*-*",
 "*display.background: lightgreen",
#ifdef USE_XAW
 "*displayb.background: black",
 "*displayb.foreground: white",
#endif
 "*keybox.background: lightgrey",
 NULL
};

static void brl_identify()
{
 LogPrint(LOG_NOTICE, XW_VERSION);
 LogPrint(LOG_INFO,   "   "XW_COPYRIGHT);
}

static void brl_display() {
 char *disp,*dispb;
 int y,x;
 Widget tmp_vbox;

 /* horizontal separation */
 hbox = XtVaCreateManagedWidget("hbox",panedWidgetClass,vbox,
   XtNorientation, XtEhorizontal,
#ifdef USE_XM
   XmNmarginHeight, 0,
   XmNmarginWidth, 0,
   XmNspacing, 0,
#endif
#ifdef USE_XAW
   XtNshowGrip,False,
#else
   XmNpaneMaximum,20,
   XmNpaneMinimum,20,
   XmNskipAdjust, True,
#endif
   XtNresize, True,
   NULL);

 /* display Label */
 disp=XtMalloc(2);
 disp[0]=' ';
 disp[1]=0;

 dispb=XtMalloc(4);
 dispb[0]=0xe0|((0x28>>4)&0x0f);
 dispb[1]=0x80|((0x28<<2)&0x3f);
 dispb[2]=0x80;
 /*
 dispb[0]='a';
 dispb[1]=0;
 */
 dispb[3]=0;

#ifdef USE_XM
 display_cs = XmStringCreateLocalized(disp);
#endif

 for (x=0;x<cols;x++) {
  /* vertical separation */
  tmp_vbox = XtVaCreateManagedWidget("tmp_vbox",panedWidgetClass,hbox,
#ifdef USE_XAW
   XtNshowGrip,False,
#else
   XmNpaneMaximum,20,
   XmNpaneMinimum,20,
   XmNskipAdjust, True,
#endif
#ifdef USE_XM
   XmNmarginHeight, 0,
   XmNmarginWidth, 0,
   XmNspacing, 0,
#endif
   XtNresize, True,
   NULL);

  for (y=0;y<lines;y++) {
   char action[] = "<Btn1Up>: route(100)";
   XtTranslations transl;

   snprintf(action,sizeof(action),"<Btn1Up>: route(%u)",y*cols+x);
   transl = XtParseTranslationTable(action);

   display[y*cols+x] = XtVaCreateManagedWidget("display",labelWidgetClass,tmp_vbox,
    XtNtranslations, transl,
#ifdef USE_XAW
    XtNshowGrip,False,
#else
    XmNpaneMaximum,20,
    XmNpaneMinimum,20,
    XmNskipAdjust, True,
#endif
#ifdef USE_XAW
    XtNlabel, disp,
#else
    XmNlabelString, display_cs,
#endif
    NULL);

#ifdef USE_XAW
   if (fontset) {
    displayb[y*cols+x] = XtVaCreateManagedWidget("displayb",labelWidgetClass,tmp_vbox,
    XtNtranslations, transl,
    XtNinternational, True,
    XNFontSet, fontset,
    XtNshowGrip,False,
    XtNlabel, dispb,
    NULL);
   }
#endif
  }
 }
#ifdef USE_XM
 XmStringFree(display_cs);
#endif
 XtFree(disp);

}

static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
 int argc = 1;
 static char *defargv[]= { "brltty", NULL };
 char **argv = defargv;
#ifdef USE_XAW
 char *def_string_return;
 char **missing_charset_list_return;
 int missing_charset_count_return;
#endif
 XtActionsRec routeAction = { "route", route };

 lines=1;
 if (*parameters[PARM_LINES]) {
  static const int minimum = 1;
  static const int maximum = MAXLINES;
  int value;
  if (validateInteger(&value, "lines", parameters[PARM_LINES], &minimum, &maximum))
   lines=value;
 }

 cols=40;
 if (*parameters[PARM_COLS]) {
  static const int minimum = 1;
  static const int maximum = MAXCOLS;
  int value;
  if (validateInteger(&value, "cols", parameters[PARM_COLS], &minimum, &maximum))
   cols=value;
 }

 brl->x=cols;
 brl->y=lines;

 if (*parameters[PARM_XTPARMS]) {
  argv = splitString(parameters[PARM_XTPARMS],' ',&argc);
  argv = reallocWrapper(argv, (argc+2) * sizeof(char *));
  memmove(argv+1,argv,(argc+1) * sizeof(char *));
  argv[0] = strdupWrapper(defargv[0]);
  argc++;
 }

 if (*parameters[PARM_MODEL])
  model = parameters[PARM_MODEL];
 
 XtSetLanguageProc(NULL, NULL, NULL);
 
 /* toplevel */
 toplevel = XtVaOpenApplication(&app_con, "Brltty",
   optionDescList, XtNumber(optionDescList),
   &argc, argv, fallback_resources,
   sessionShellWidgetClass,
   XtNallowShellResize, True,
   NULL);

 if (argv != defargv)
  deallocateStrings(argv);

 XtAppAddActions(app_con,&routeAction,1);

 /* vertical separation */
 vbox = XtVaCreateManagedWidget("vbox",panedWidgetClass,toplevel,
#ifdef USE_XM
   XmNmarginHeight, 0,
   XmNmarginWidth, 0,
   XmNspacing, 1,
#endif
   XtNresize, True,
   NULL);

#ifdef USE_XAW
 if (!(fontset = XCreateFontSet(XtDisplay(toplevel),"-*-clearlyu-*-*-*-*-17-*-*-*-*-*-iso10646-1", &missing_charset_list_return, &missing_charset_count_return, &def_string_return)))
  LogPrint(LOG_ERR,"Error while loading braille font");
#endif
 
 brl_display();

 if (!model || strcmp(model,"bare")) {
   /* key box */
   keybox = XtVaCreateManagedWidget("keybox",formWidgetClass,vbox,
#ifdef USE_XAW
     XtNdefaultDistance,0,
#endif
     NULL);
   if (model && !strcmp(model,"vs"))
    createKeyButtons(buttons_vs);
   else
    createKeyButtons(buttons_simple);
 }

 /* go go go */
 XtRealizeWidget(toplevel);

 return 1;
}

static void brl_close(BrailleDisplay *brl)
{
#ifdef USE_XAW
 if (fontset)
  XFreeFontSet(XtDisplay(toplevel),fontset);
#endif
 XtDestroyApplicationContext(app_con);
}

static void brl_writeWindow(BrailleDisplay *brl)
{
#ifdef USE_XAW
 static unsigned char displayed[WHOLESIZE];
 int i;
 unsigned char data[4];
 char c;

 if (!displayb[0] || !memcmp(brl->buffer,displayed,brl->y*brl->x)) return;
 memcpy(displayed,brl->buffer,brl->y*brl->x);

 for (i=0;i<brl->y*brl->x;i++) {
  c = brl->buffer[i];
  brl->buffer[i] =
     (!!(c&BRL_DOT1))<<0
    |(!!(c&BRL_DOT2))<<1
    |(!!(c&BRL_DOT3))<<2
    |(!!(c&BRL_DOT4))<<3
    |(!!(c&BRL_DOT5))<<4
    |(!!(c&BRL_DOT6))<<5
    |(!!(c&BRL_DOT7))<<6
    |(!!(c&BRL_DOT8))<<7;
  data[0]=0xe0|((0x28>>4)&0x0f);
  data[1]=0x80|((0x28<<2)&0x3f)|(brl->buffer[i]>>6);
  data[2]=0x80                 |(brl->buffer[i]&0x3f);
  data[3]=0;

  XtVaSetValues(displayb[i],
   XtNlabel, data,
   NULL);
 }
#endif
}

static void brl_writeVisual(BrailleDisplay *brl)
{
 static unsigned char displayed[WHOLESIZE];
 int i;
 unsigned char data[2];

 if (!memcmp(brl->buffer,displayed,brl->y*brl->x)) return;
 memcpy(displayed,brl->buffer,brl->y*brl->x);

 for (i=0;i<brl->y*brl->x;i++) {
  data[0]=brl->buffer[i];
  if (data[0]==0) data[0]=' ';
  data[1]=0;

#ifdef USE_XM
  display_cs = XmStringCreateLocalized(data);
#endif
  XtVaSetValues(display[i],
#ifdef USE_XAW
   XtNlabel, data,
#else
   XmNlabelString, display_cs,
#endif
   NULL);
#ifdef USE_XM
  XmStringFree(display_cs);
#endif
 }
}

static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *s)
{
 /* TODO: another label */
}
