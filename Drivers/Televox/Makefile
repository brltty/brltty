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
# Makefile for BRLTTY Televox drivers
#
# It is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and BRLDEV
# from the main Makefile...
###############################################################################

DRIVER_CODE = tv
DRIVER_NAME = Televox
SPEECH_MODELS = 
include ../driver.mk

speech.o: speech.c speech.h ../spk.h ../spk_driver.h ../misc.h ../config.h
	$(CC) $(SPK_CFLAGS) -c speech.c

