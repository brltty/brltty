###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-1998 by The BRLTTY Team, All rights reserved.
#
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <s.doyon@videotron.ca>
# Nikhil Nair <nn201@cus.cam.ac.uk>
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nicolas Pitre <nico@cam.org>.
#
###############################################################################

###############################################################################
# Default Makefile for BRLTTY Braille/speech drivers
#
# This can be overridden by a Makefile in the relevant subdirectory.
# It is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and BRLDEV
# on the command line.
#
# Targets are brl.o and/or speech.o.
###############################################################################

brl.o: brl.c brlconf.h ../brl.h ../misc.h ../scr.h ../config.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brl.c

speech.o: speech.c ../speech.h ../config.h
	$(CC) $(CFLAGS) -c speech.c
