###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2001 by The BRLTTY Team, All rights reserved.
#
# Web Page: http://www.cam.org/~nico/brltty
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
###############################################################################

###############################################################################
# Makefile for the Tieman B.V. CombiBraille driver
# $Id: Makefile,v 1.1 1996/09/25 00:26:28 nn201 Exp $
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER = cb
include ../driver.mk

brl.o: brl.c ../brl.h ../misc.h brlconf.h ../brl_driver.h tables.h
	$(CC) $(BRL_CFLAGS) -c brl.c

speech.o: speech.c speech.h ../spk.h ../spk_driver.h ../misc.h brlconf.h
	$(CC) $(SPK_CFLAGS) -c speech.c

brl-lib-name:
	echo -e "cb\tTieman CombiBraille" >> $(BRLNAMES)

spk-lib-name:
	echo -e "cb\tTieman CombiBraille" >> $(SPKNAMES)

