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
# Makefile for BRLTTY's driver for Tactilog's LogText.
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER_CODE = lt
DRIVER_NAME = LogText
BRAILLE_MODELS = 32
include ../driver.mk

brl.o: brl.c brlconf.h input.h output.h ../brl.h ../misc.h ../brl_driver.h
	$(CC) $(BRL_CFLAGS) -c brl.c

