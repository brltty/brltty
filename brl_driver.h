/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2001 by The BRLTTY Team. All rights reserved.
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

/* this header file is used to create the driver structure */
/* for a dynamically loadable braille display driver. */
/* BRLNAME, HELPNAME, and PREFSTYLE must be defined - see brlconf.h */

/* Routines provided by this braille display driver. */
static void identbrl (void);/* print start-up messages */
static void initbrl (char **parameters, brldim *, const char *); /* initialise Braille display */
static void closebrl (brldim *); /* close braille display */
static void writebrl (brldim *); /* write to braille display */
static int readbrl (DriverCommandContext);	/* get key press from braille display */
static void setbrlstat (const unsigned char *);	/* set status cells */

#ifdef BRLPARMS
static char *brl_parameters[] = {BRLPARMS, NULL};
#endif

braille_driver brl_driver = 
{
  BRLNAME,
  BRLDRIVER,
#ifdef BRLPARMS
  brl_parameters,
#else
  NULL,
#endif
  HELPNAME,
  PREFSTYLE,
  identbrl,
  initbrl,
  closebrl,
  writebrl,
  readbrl,
  setbrlstat
};
