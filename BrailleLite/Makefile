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
# Makefile for the BrailleLite driver
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER_CODE = bl
DRIVER_NAME = BrailleLite
BRAILLE_MODELS = 18/40
SPEECH_MODELS = 
HELPTARGETS = brlttyhelp0.txt brlttyhelp1.txt
include ../driver.mk

brl.o: brl.c ../brl.h ../misc.h ../scr.h ../message.h \
	../config.h brlconf.h ../brl_driver.h bindings.h
	$(CC) $(BRL_CFLAGS) -c brl.c

speech.o: speech.c speech.h ../spk.h ../spk_driver.h ../misc.h brlconf.h
	$(CC) $(SPK_CFLAGS) -c speech.c

brlttyhelp0.txt: brltty_genhelp.txt
	sed 's/<x20>\(.*\)$$/\1/' < brltty_genhelp.txt \
	| grep -Ev '<x40>.*$$' > brlttyhelp0.txt

brlttyhelp1.txt: brltty_genhelp.txt
	sed 's/<x40>\(.*\)$$/\1/' < brltty_genhelp.txt \
	| grep -Ev '<x20>.*$$' > brlttyhelp1.txt

distclean:
	rm -f brlttyhelp0.txt brlttyhelp1.txt
