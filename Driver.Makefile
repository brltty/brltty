###############################################################################
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Nikhil Nair <nn201@cus.cam.ac.uk>
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <doyons@jsp.umontreal.ca>
#
# Version 1.0.2, 17 September 1996
#
# Copyright (C) 1995, 1996 by Nikhil Nair and others.  All rights reserved.
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nikhil Nair <nn201@cus.cam.ac.uk>.
###############################################################################

###############################################################################
# Default Makefile for BRLTTY Braille/speech drivers
# $Id: Driver.Makefile,v 1.1 1996/09/25 00:26:28 nn201 Exp $
#
# This can be overridden by a Makefile in the relevant subdirectory.
# It is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and BRLDEV
# on the command line.
#
# Targets are brl.o and/or speech.o.
###############################################################################

brl.o: brl.c brlconf.h ../brl.h ../misc.h ../scr.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brl.c

speech.o: speech.c ../speech.h
	$(CC) $(CFLAGS) -c speech.c
