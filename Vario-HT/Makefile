###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
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
# Makefile for BRLTTY Vario-HT driver
# All variables are defined in the main Makefile...
#
###############################################################################

IFLAGS = -I..
DRIVER = vh
include ../driver.mk

variolow.o:	variolow.c variolow.h
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -c variolow.c

vario.o:	vario.c brlconf.h ../brl_driver.h ../misc.h
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -c vario.c

brl.o:	vario.o	variolow.o
	$(LD) -r -o brl.o variolow.o vario.o 

brl-lib-name:
	echo -e "vh\tVario Emul. 2" >> $(BRLNAMES)
