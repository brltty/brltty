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
# Makefile for BRLTTY Alva driver
# All variables are defined in the main Makefile...
#
###############################################################################


# Uncomment next line if you want to use the parallel port interface
# and you configured USE_PARALLEL_PORT to 1 in brlconf.h
# Comment it out if you are compiling on a non-i386 architecture.
# Note that this option uses an extra library.  See README for details.
#ALVALIB = alva_api.library/alva_api.a
ALVALIB =

DRIVER = al
include ../driver.mk

brlmain.o: brlmain.cc brlconf.h ../brl_driver.h ../brl.h ../misc.h ../scr.h ../config.h
	$(COMPCPP) $(BRL_CFLAGS) -c brlmain.cc

brl.o: brlmain.o $(ALVALIB)
	$(LD) -r -o brl.o brlmain.o $(ALVALIB)

speech.o: speech.c speech.h ../spk.h ../spk_driver.h ../misc.h brlconf.h
	$(CC) $(SPK_CFLAGS) -c speech.c

brl-lib-name:
	echo -e "al\tAlva (ABT3xx/Delphi)" >> $(BRLNAMES)

spk-lib-name:
	echo -e "al\tAlva (Delphi)" >> $(SPKNAMES)

