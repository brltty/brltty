###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-1999 by The BRLTTY Team, All rights reserved.
#
# Nicolas Pitre <nico@cam.org>
# Stéphane Doyon <s.doyon@videotron.ca>
# Nikhil Nair <nn201@cus.cam.ac.uk>
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
# Makefile for BRLTTY Eco driver
# All variables are defined in the main Makefile...
#
###############################################################################

HELP = ec
HELPNAME = brltty-$(HELP).hlp

help:
	../txt2hlp $(HELPNAME) brlttyh*.txt

brl.o: brl.c brlconf.h ../brl.h ../misc.h ../scr.h ../config.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' \
	'-DHELPNAME="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"' -c brl.c

# shared library
# name of library - 2 char
LIB_DESC = ec
LIB_NAME = $(LIB_SO_NAME)$(LIB_DESC).so.$(LIB_VER)

lib: brl.o
	$(CC) -g -shared -Wl,-soname,$(LIB_NAME) -o $(LIB_NAME) brl.o -lc
	touch lib

