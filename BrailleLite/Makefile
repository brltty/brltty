###############################################################################
#
# BRLTTY - Access software for Unix for a blind person
#          using a soft Braille terminal
#
# Copyright (C) 1995-2000 by The BRLTTY Team, All rights reserved.
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
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER = bl
LIBNAME18 = $(LIB_SO_NAME)b1.so.$(LIB_VER)
LIBFILE18 = $(LIBDIR)/$(LIBNAME18)
LIBNAME40 = $(LIB_SO_NAME)b4.so.$(LIB_VER)
LIBFILE40 = $(LIBDIR)/$(LIBNAME40)
LIB_TARGETS = $(LIBFILE18) $(LIBFILE40)
include ../driver.mk

TARGETS = brl.c ../brl.h ../misc.h ../inskey.h ../message.h \
	../config.h brlconf.h ../driver.h bindings.h

brl.o: $(TARGETS)
	$(CC) $(DRIVER_CFLAGS) -c brl.c

brl18.o: $(TARGETS)
	$(CC) $(DRIVER_CFLAGS) -DBLITE_SIZE=18 -o brl18.o -c brl.c

$(LIBFILE18): brl18.o
	$(CC) -shared -Wl,-soname,$(LIBNAME18) -o $(LIBFILE18) brl18.o -lc

brl40.o: $(TARGETS)
	$(CC) $(DRIVER_CFLAGS) -DBLITE_SIZE=40 -o brl40.o -c brl.c

$(LIBFILE40): brl40.o
	$(CC) -shared -Wl,-soname,$(LIBNAME40) -o $(LIBFILE40) brl40.o -lc

speech.o: speech.c ../speech.h brlconf.h speech.h
	$(CC) $(DRIVER_CFLAGS) -c speech.c
