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
# Makefile for BRLTTY Vario driver
# All variables are defined in the main Makefile...
#
###############################################################################

IFLAGS = -I..
LD = ld

DRIVER = va
include ../driver.mk

variolow.o:	variolow.c variolow.h
	$(CC) $(DRIVER_CFLAGS) $(IFLAGS) -c variolow.c

vario.o:	vario.c brlconf.h ../driver.h
	$(CC) $(DRIVER_CFLAGS) $(IFLAGS) -c vario.c
	
brl.o:	vario.o	variolow.o
	$(LD) -r -o brl.o variolow.o vario.o 

