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
# Makefile for BRLTTY VideoBraille driver
# All variables are defined in the main Makefile...
#
###############################################################################

IFLAGS = -I..
DRIVER_CODE = vd
DRIVER_NAME = VideoBraille
BRAILLE_MODELS =  40
include ../driver.mk

vblow.o:	vblow.c
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -O2 -c vblow.c

vb.o:	vb.c brlconf.h ../brl_driver.h ../misc.h
	$(CC) $(BRL_CFLAGS) $(IFLAGS) -c vb.c
	
brl.o:	vb.o	vblow.o
	$(LD) -r -O2 -o brl.o vblow.o vb.o 

