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
# This software is maintained by Nicolas Pitre <nico@cam.org>.
#
###############################################################################

###############################################################################
#
# Makefile for BRLTTY EuroBraille driver
# All variables are defined in the main Makefile...
#
###############################################################################

DRIVER = eu
include ../driver.mk

brl.o: brl.c brlconf.h ../driver.h ../brl.h ../misc.h ../scr.h ../config.h
	$(CC) $(DRIVER_CFLAGS) -c brl.c
