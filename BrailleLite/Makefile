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
###############################################################################

###############################################################################
# Makefile for the BrailleLite driver
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, LDLIBS and
# BRLDEV
###############################################################################

HELP = bl
HELPNAME = brltty-$(HELP).hlp

help:
	../txt2hlp $(HELPNAME) brlttyh*.txt

brl.o:	brl.c ../brl.h ../misc.h ../inskey.h ../message.h ../config.h \
	brlconf.h bindings.h
	$(CC) $(CFLAGS) '-DBRLDEV="$(BRLDEV)"' '-DBLITE_SIZE=$(TYPE)' \
        '-DHELPNAME="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"' \
        -c brl.c

speech.o: speech.c ../speech.h brlconf.h speech.h
	$(CC) $(CFLAGS) -c speech.c

# shared library
# name of library - 18 (b1) and 40 (b4)

LIB_NAME = $(LIB_SO_NAME)$(LIB_DESC).so.$(LIB_VER)

lib: brl.c ../brl.h ../misc.h ../inskey.h ../message.h ../config.h \
	brlconf.h bindings.h
	rm -f brl.o 
	$(MAKE) lib1 LIB_DESC=b1 TYPE=18
	rm -f brl.o 
	$(MAKE) lib1 LIB_DESC=b4 TYPE=40
	touch lib

lib1: brl.o
	$(CC) -g -shared -Wl,-soname,$(LIB_NAME) -o $(LIB_NAME) brl.o -lc
