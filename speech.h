/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <doyons@jsp.umontreal.ca>
 *
 * Version 1.0.2, 17 September 1996
 *
 * Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
 */

/* speech.h - Header file for the speech library
 * $Id: speech.h,v 1.2 1996/09/24 01:04:27 nn201 Exp $
 */


void identspk (void);		/* identify speech driver */
void initspk (void);		/* initialise speech */
void say (unsigned char *buffer, int len);	/* speak text */
void mutespk (void);		/* mute speech */
void closespk (void);		/* close speech driver */
