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
# Makefile for the BrailleLite driver
#
# This Makefile is passed the variables CC, CFLAGS, LD, LDFLAGS, and LDLIBS
###############################################################################

DRIVER = bl
BRL18_NAME = $(BRL_SO_NAME)b1.so.$(LIB_VER)
BRL18_FILE = $(LIBDIR)/$(BRL18_NAME)
BRL40_NAME = $(BRL_SO_NAME)b4.so.$(LIB_VER)
BRL40_FILE = $(LIBDIR)/$(BRL40_NAME)
BRL_FILES = $(BRL18_FILE) $(BRL40_FILE)
include ../driver.mk

TARGETS = brl.c ../brl.h ../misc.h ../inskey.h ../message.h \
	../config.h brlconf.h ../brl_driver.h bindings.h

brl.o: $(TARGETS)
	$(CC) $(BRL_CFLAGS) -c brl.c

brl18.o: $(TARGETS)
	$(CC) $(BRL_CFLAGS) -DBLITE_SIZE=18 -o brl18.o -c brl.c

$(BRL18_FILE): brl18.o
	$(LD) $(BRL_LFLAGS) -soname $(BRL18_NAME) -o $(BRL18_FILE) brl18.o -lc

brl40.o: $(TARGETS)
	$(CC) $(BRL_CFLAGS) -DBLITE_SIZE=40 -o brl40.o -c brl.c

$(BRL40_FILE): brl40.o
	$(LD) $(BRL_LFLAGS) -soname $(BRL40_NAME) -o $(BRL40_FILE) brl40.o -lc

speech.o: speech.c speech.h ../spk.h ../spk_driver.h ../misc.h brlconf.h
	$(CC) $(SPK_CFLAGS) -c speech.c

brl-lib-name:
	echo -e "b1\tBrailleLite 18" >> $(BRLNAMES)
	echo -e "b4\tBrailleLite 40" >> $(BRLNAMES)

spk-lib-name:
	echo -e "bl\tBrailleLite" >> $(SPKNAMES)

