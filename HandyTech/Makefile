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
# Makefile for BRLTTY Handy driver
# All variables are defined in the main Makefile...
#
###############################################################################

DRIVER_CODE = ht
DRIVER_NAME = HandyTech
BRAILLE_MODELS = Modular 20/40/80, Wave 40, Star 40/80, Bookworm
include ../driver.mk


brlmain.o: brlmain.cc brlconf.h ../brl.h ../misc.h ../scr.h ../config.h
	$(COMPCPP) $(BRL_CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brlmain.cc

brl.o: brlmain.o
	$(LD) -r -o brl.o brlmain.o

