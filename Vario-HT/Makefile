###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2002 by The BRLTTY Team. All rights reserved.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

###############################################################################
#
# Makefile for BRLTTY Vario-HT driver
# All variables are defined in the main Makefile...
#
###############################################################################

IFLAGS = -I..
DRIVER_CODE = vh
DRIVER_NAME = Vario/RBT (emulation 2)
BRAILLE_MODELS =  40/80
include ../driver.mk

variolow.o:	variolow.c variolow.h
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -c variolow.c

vario.o:	vario.c brlconf.h ../brl_driver.h ../misc.h
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -c vario.c

brl.o:	vario.o	variolow.o
	$(LD) -r -o brl.o variolow.o vario.o 

