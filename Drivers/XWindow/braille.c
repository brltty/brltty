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
#define VERSION "BRLTTY driver for X, version 0.1, 2004"
#define COPYRIGHT "Copyright Samuel Thibault <samuel.thibault@ens-lyon.org>"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "Programs/brl.h"
#include "Programs/misc.h"
#include "Programs/scr.h"
#include "Programs/message.h"

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
#error unknown xaw package
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
#error unknown xaw prototype
#endif /* USE_ */

typedef enum {
  PARM_DISPLAY,
  PARM_TERM,
  PARM_LINES,
  PARM_COLS,
  PARM_MODEL
} DriverParameter;
#define BRLPARMS "display", "term", "lines", "cols", "model"

#include "Programs/brl_driver.h"
#include "braille.h"

#define MAXLINES 3
#define MAXCOLS 80
#define WHOLESIZE (MAXLINES * MAXCOLS)
static int cols,lines;
static char *model;

#define BUTWIDTH 48
#define BUTHEIGHT 32

static Widget toplevel,vbox,keybox,display;
static XtAppContext app_con;
#ifdef USE_XM
static XmString display_cs;
#endif

static long keypressed;

static void KeyPressCB(Widget w, XtPointer closure, XtPointer callData) {
 LogPrint(LOG_DEBUG,"keypress(%p)",closure);
 keypressed=(long)closure;
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

static void createKeyButtons(void) {
 crKeyBut("Up",	   BRL_CMD_LNUP,   1, BUTWIDTH+1,     0);
 crKeyBut("Left",  BRL_CMD_FWINLT, 1, 0,              BUTHEIGHT+1);
 crKeyBut("Home",  BRL_CMD_HOME,   0, BUTWIDTH+1,     BUTHEIGHT+1);
 crKeyBut("Right", BRL_CMD_FWINRT, 1, 2*(BUTWIDTH+1), BUTHEIGHT+1);
 crKeyBut("Down",  BRL_CMD_LNDN,   1, BUTWIDTH+1,     2*(BUTHEIGHT+1));
 crKeyBut("alt-a", BRL_FLG_CHAR_META | BRL_BLK_PASSCHAR | 'a', 0, 0, 3*(BUTHEIGHT+1));
 crKeyBut("ctrl-a", BRL_FLG_CHAR_CONTROL | BRL_BLK_PASSCHAR | 'a', 0, BUTWIDTH+1, 3*(BUTHEIGHT+1));
 crKeyBut("a", BRL_BLK_PASSCHAR | 'a', 0, 2*(BUTWIDTH)+1, 3*(BUTHEIGHT+1));
 crKeyBut("A", BRL_BLK_PASSCHAR | 'A', 0, 3*(BUTWIDTH)+1, 3*(BUTHEIGHT+1));
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

static XrmOptionDescRec optionDescList[] = {
};

static char *fallback_resources[] = {
 "*display.background: lightgreen",
 "*keybox.background: lightgrey",
 NULL
};

static void brl_identify()
{
 LogPrint(LOG_NOTICE, VERSION);
 LogPrint(LOG_INFO,   COPYRIGHT);
}

static int brl_open(BrailleDisplay *brl, char **parameters, const char *device)
{
 char *term;
 int argc = 1;
 char *argv[4] = { "brltty", NULL };
 char *disp;
 int y;

 lines=1;
 if (*parameters[PARM_LINES]) {
  static const int minimum = 1;
  static const int maximum = MAXLINES;
  int value;
  if (validateInteger(&value, "lines", parameters[PARM_LINES], &minimum, &maximum))
   lines=value;
 }

 cols=40;
 lines=1;

 if (*parameters[PARM_COLS]) {
  static const int minimum = 1;
  static const int maximum = MAXCOLS;
  int value;
  if (validateInteger(&value, "cols", parameters[PARM_COLS], &minimum, &maximum))
   cols=value;
 }

 brl->x=cols;
 brl->y=lines;

 if (*parameters[PARM_DISPLAY]) {
  argv[argc++]="-display";
  argv[argc++]=parameters[PARM_DISPLAY];
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

 /* vertical separation */
 vbox = XtVaCreateManagedWidget("vbox",panedWidgetClass,toplevel,
#ifdef USE_XM
   XmNmarginHeight, 0,
   XmNmarginWidth, 0,
   XmNspacing, 1,
#endif
   XtNresize, True,
   NULL);

 /* display Label */
 disp=XtMalloc((cols+1)*lines);
 for (y=0;y<lines;y++) {
  memset(disp+y*(cols+1),' ',cols);
  disp[(y+1)*(cols+1)-1]='\n';
 }
 disp[lines*(cols+1)]=0;
#ifdef USE_XM
 display_cs = XmStringCreateLocalized(disp);
#endif
 display = XtVaCreateManagedWidget("display",labelWidgetClass,vbox,
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
 XtFree(disp);
#ifdef USE_XM
 XmStringFree(display_cs);
#endif


 if (!model || strcmp(model,"bare")) {
   /* key box */
   keybox = XtVaCreateManagedWidget("keybox",formWidgetClass,vbox,
#ifdef USE_XAW
     XtNdefaultDistance,0,
#endif
     NULL);
   createKeyButtons();
 }

 /* go go go */
 XtRealizeWidget(toplevel);

 if (*parameters[PARM_TERM])
  term=parameters[PARM_TERM];

 return 1;
}

static void brl_close(BrailleDisplay *brl)
{
 XtDestroyApplicationContext(app_con);
}

static void brl_writeWindow(BrailleDisplay *brl)
{
 unsigned char data[brl->x*brl->y];
 static unsigned char displayed[WHOLESIZE];
 int y;
 unsigned char *c;

 for (y=0;y<brl->y;y++) {
  memcpy(data+y*(brl->x+1),brl->buffer+y*brl->x,brl->x);
  data[(y+1)*(brl->x+1)-1]='\n';
 }

 c=data;
 while ((c=memchr(c,0,(data+brl->y*(brl->x+1))-c))) *c++=' ';

 data[brl->y*(brl->x+1)-1]=0;

 if (!memcmp(data,displayed,brl->y*(brl->x+1)))
  return;

 memcpy(displayed,data,brl->y*(brl->x+1));

#ifdef USE_XM
 display_cs = XmStringCreateLocalized(displayed);
#endif
 XtVaSetValues(display,
#ifdef USE_XAW
   XtNlabel, displayed,
#else
   XmNlabelString, display_cs,
#endif
   NULL);
#ifdef USE_XM
 XmStringFree(display_cs);
#endif
}

static void brl_writeStatus(BrailleDisplay *brl, const unsigned char *s)
{
 /* TODO: another label */
}
