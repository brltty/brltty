###############################################################################
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Nicolas Pitre <nico@cam.org>
# Stephane Doyon <doyons@jsp.umontreal.ca>
# Nikhil Nair <nn201@cus.cam.ac.uk>
#
# Copyright (C) 1995 1999 by the BRLTTY team.  All rights reserved.
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# This software is maintained by Nicolas Pitre <nico@cam.org>
###############################################################################

###############################################################################
# Makefile for the Papenmeier Terminal
# This software is maintained by August Hörandl <hoerandl@elina.htlw1.ac.at>
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and
# BRLDEV from the main Makefile
###############################################################################

HELP = pm
HELPNAME = brltty-$(HELP).hlp

help:
	../txt2hlp $(HELPNAME) brlttyh*.txt

brl.o: brl.c ../brl.h ../scr.h ../misc.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' \
	'-DHELPNAME="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"' -c brl.c

serial: serial.c ../brl.h brl.c ../text.auto.h ../misc.c 
	$(CC) $(CFLAGS) '-DBRLDEV="/dev/ttyS1"' serial.c ../misc.c -o serial -lncurses 

# shared library
# name of library - 2 char
LIB_DESC = pm
LIB_NAME = $(LIB_SO_NAME)$(LIB_DESC).so.$(LIB_VER)

lib: brl.o
	$(CC) -g -shared -Wl,-soname,$(LIB_NAME) -o $(LIB_NAME) brl.o -lc
	touch lib
