###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
#
# Nicolas Pitre <nico@cam.org>
# Stéphane Doyon <s.doyon@videotron.ca>
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
# Makefile for BRLTTY general speech drivers
#
# It is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and BRLDEV
# from the main Makefile...
###############################################################################

speech.o: speech.c speech.h ../speech.h ../config.h
	$(CC) $(CFLAGS) -c speech.c
