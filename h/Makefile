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
#
# Makefile for BRLTTY Handy driver
# All variables are defined in the main Makefile...
#
###############################################################################

DRIVER_CODE = ht
DRIVER_NAME = HandyTech
BRAILLE_MODELS = 
include ../driver.mk


# Uncomment next line if you want to use the parallel port interface
# and you configured USE_PARALLEL_PORT to 1 in brlconf.h
# Comment it out if you are compiling on a non-i386 architecture.
# Note that this option uses an extra library.  See README for details.


LD = ld

brlmain.o: brlmain.cc brlconf.h ../brl.h ../misc.h ../scr.h ../config.h
	$(COMPCPP) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' -c brlmain.cc

brl.o: brlmain.o
	$(LD) -r -o brl.o brlmain.o

