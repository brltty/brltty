###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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
###############################################################################

###############################################################################
# Makefile for the BrailleLite driver
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and
# BRLDEV
###############################################################################

brl.o:	brl.c ../brl.h ../misc.h ../inskey.h ../message.h ../config.h \
	brlconf.h bindings.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brl.c

speech.o: speech.c ../speech.h brlconf.h speech.h
	$(CC) $(CFLAGS) -c speech.c
