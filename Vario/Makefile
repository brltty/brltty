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
# This software is maintained by Nicolas Pitre <nico@cam.org>.
#
###############################################################################

###############################################################################
#
# Makefile for BRLTTY BAUM Vario Emulation 1 driver
# All variables are defined in the main Makefile...
#
###############################################################################

DRIVER = va
include ../driver.mk

brl.o: brl.c brlconf.h ../brl_driver.h ../brl.h ../misc.h ../scr.h ../inskey.h ../beeps.h ../config.h
	$(CC) $(BRL_CFLAGS) -c brl.c

brl-lib-name:
	echo -e "va\tVario Emul. 1 (40/80)" >> $(BRLNAMES)


