###############################################################################
# BrlTty - A daemon providing access to the Linux console (when in text
#          mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
#
# BrlTty comes with ABSOLUTELY NO WARRANTY.
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
# Makefile for the Tieman B.V. CombiBraille driver
# $Id: Makefile,v 1.1 1996/09/25 00:26:28 nn201 Exp $
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER_CODE = cb
DRIVER_NAME = CombiBraille
BRAILLE_MODELS = 25/45/85
SPEECH_MODELS = 
include ../driver.mk

brl.o: brl.c ../brl.h ../misc.h brlconf.h ../brl_driver.h tables.h
	$(CC) $(BRL_CFLAGS) -c brl.c

speech.o: speech.c speech.h ../spk.h ../spk_driver.h ../misc.h brlconf.h
	$(CC) $(SPK_CFLAGS) -c speech.c

