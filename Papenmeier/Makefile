###############################################################################
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <doyons@jsp.umontreal.ca>
# Nikhil Nair <nn201@cus.cam.ac.uk>
#
# Copyright (C) 1995-2000 by the BRLTTY team.  All rights reserved.
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nicolas Pitre <nico@cam.org>
###############################################################################

###############################################################################
# Makefile for the Papenmeier Terminal
# This software is maintained by August Hörandl <hoerandl@elina.htlw1.ac.at>
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and
# BRLDEV from the main Makefile
###############################################################################

brl.o: brl.c ../brl.h ../scr.h ../misc.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brl.c

serial: serial.c ../brl.h brl.c ../text.auto.h
	$(CC) $(CFLAGS) '-DBRLDEV="/dev/ttyS1"' serial.c -o serial -lncurses 


