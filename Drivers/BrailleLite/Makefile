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
###############################################################################

DRIVER_CODE = bl
DRIVER_NAME = BrailleLite
BRAILLE_MODELS = 18/40
SPEECH_MODELS = 
HELP_DEPS = brlttyhelp0.txt brlttyhelp1.txt

include setup.mk
include $(TOP_DIR)/driver.mk

braille.o:
	$(CC) $(BRL_CFLAGS) -c braille.c

speech.o:
	$(CC) $(SPK_CFLAGS) -c speech.c

brlttyhelp0.txt: brltty_genhelp.txt
	sed 's/<x20>\(.*\)$$/\1/' < brltty_genhelp.txt \
	| grep -v '<x40>' > brlttyhelp0.txt

brlttyhelp1.txt: brltty_genhelp.txt
	sed 's/<x40>\(.*\)$$/\1/' < brltty_genhelp.txt \
	| grep -v '<x20>' > brlttyhelp1.txt

clean::
	-rm -f $(HELP_DEPS)
