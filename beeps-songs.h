/*
 * BRLTTY - Access software for Unix for a blind person
 *          using a soft Braille terminal
 *
 * Version 1.9.0, 06 April 1998
 *
 * Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
 *
 * Nikhil Nair <nn201@cus.cam.ac.uk>
 * Nicolas Pitre <nico@cam.org>
 * Stephane Doyon <s.doyon@videotron.ca>
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * This software is maintained by Nicolas Pitre <nico@cam.org>.
 */

/* beeps-songs.h - definitions of different types of beeps
 * $Id: beeps-songs.h,v 1.3 1996/09/24 01:04:24 nn201 Exp $
 */

int snd_link[] =
{1400, 7, 1500, 7, 1600, 10, 0};
int snd_unlink[] =
{1600, 7, 1500, 7, 1400, 7, 0};
/*int snd_wrap_down[] = {8000, 5, 4000, 7, 2000, 9, 1000, 9, 500, 5,0}; */
int snd_wrap_down[] =
{8000, 10, 4000, 7, 2000, 8, 1000, 6, 0};
/*int snd_wrap_up[] = {1000, 4, 2000, 25, 4000, 25, 8000, 20,0}; */
int snd_wrap_up[] =
{8000, 10, 4000, 7, 2000, 8, 1000, 6, 0};
/*int snd_bounce[] = {3000, 30, 2260, 20, 4520, 18,0}; */
int snd_bounce[] =
{500, 5, 1000, 6, 2000, 8, 4000, 7, 8000, 10, 0};
int snd_freeze[] =
{5000, 5, 4800, 5, 4600, 5, 4400, 5, 4200, 5, 4000, 5, 3800, 5,
 3600, 5, 3400, 5, 3200, 5, 3000, 5, 2800, 5, 2600, 5, 2400, 5,
 2200, 5, 2000, 5, 1800, 5, 1600, 5, 1400, 5, 1200, 5, 1000, 5,
 800, 5, 600, 5, 0};
int snd_unfreeze[] =
{800, 5, 1000, 5, 1200, 5, 1400, 5, 1600, 5, 1800, 5, 2000, 5,
 2200, 5, 2400, 5, 2600, 5, 2800, 5, 3000, 5, 3200, 5, 3400, 5,
 3600, 5, 3800, 5, 4000, 5, 4200, 5, 4400, 5, 4600, 5, 4800, 5,
 5000, 5, 0};
/*int snd_cut_beg[] = {4000, 10, 3000, 10, 4000, 10, 3000, 10, 4000, 10, 3000,
   10, 4000, 10, 3000, 10, 2000, 120, 1000, 10, 0}; */
/*int snd_cut_end[] = {4000, 10, 3000, 10, 4000, 10, 3000, 10, 4000, 10, 3000,
   10, 4000, 10, 3000, 10, 1000, 70, 2000, 30, 0}; */
int snd_cut_beg[] =
{2000, 40, 1000, 20, 0};
int snd_cut_end[] =
{1000, 50, 2000, 30, 0};
int snd_toggleon[] =
{2000, 30, 1, 30, 1500, 30, 1, 30, 1000, 40, 0};
int snd_toggleoff[] =
{1000, 20, 1, 30, 1500, 20, 1, 30, 2200, 20, 0};
int snd_done[] =
{2000, 40, 1, 30, 2000, 40, 1, 40, 2000, 140, 1, 20, 1500, 50, 0};
int snd_skip[] =
{2000, 10, 0, 40, 0};
int snd_skipmore[] =
{2100, 30, 0, 20, 0};
