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

#ifndef _CONTRACT_H
#define _CONTRACT_H

void *CompileContractionTable (const char *name);
int DestroyContractionTable (void *table);
int TranslateContracted (
			 void *tbl, /* Pointer to translation table */
			 const char *source, /* What is to be translated */
			 int *SourceLen, /* Its length */
			 char *target, /* Where the translation is to go */
			 int *TargetLen, /* length of this area */
			 int *offsets, /* Array of offsets of translated chars in source */
			 int cursorPos); /* Position of coursor in source */

#endif /* !defined(_CONTRACT_H) */
