###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
#
# Web Page: http://www.cam.org/~nico/brltty
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
# Makefile for BRLTTY Videobraille driver
# All variables are defined in the main Makefile...
#
###############################################################################

IFLAGS = -I..
DRIVER = vd
include ../driver.mk

vblow.o:	vblow.c
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -O2 -c vblow.c

vb.o:	vb.c brlconf.h ../brl_driver.h ../misc.h
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -c vb.c
	
brl.o:	vb.o	vblow.o
	$(LD) -r -O2 -o brl.o vblow.o vb.o 

brl-lib-name:
	echo -e "vd\tVideobraille" >> $(BRLNAMES)

