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
#
# Sub-makefile included by makefile for each BRLTTY device driver.
# It defines the "help" and "lib" targets.
# All variables are defined in the main Makefile...
#
###############################################################################

HELPDIR = ../help
LIBDIR = ../lib
BRLNAMES = $(LIBDIR)/brltty-brl.lst
SPKNAMES = $(LIBDIR)/brltty-spk.lst

HELPNAME = brltty-$(DRIVER_CODE).hlp
HELPFILE = $(HELPDIR)/$(HELPNAME)
ifeq ($(HELPTARGETS),)
   HELPTARGETS = brlttyh*.txt
endif
$(HELPFILE): $(HELPTARGETS)
	../txt2hlp $(HELPFILE) brlttyh*.txt

braille-help: $(HELPFILE)

BRL_CFLAGS = $(LIB_CFLAGS) '-DBRLDRIVER="$(DRIVER_CODE)"' '-DBRLHELP="$(PREFIX)$(DATA_DIR)/$(HELPNAME)"'
BRL_LFLAGS = -shared
BRL_SO_NAME = $(LIB_SO_NAME)b
BRL_NAME = $(BRL_SO_NAME)$(DRIVER_CODE).so.$(LIB_VER)
BRL_FILE = $(LIBDIR)/$(BRL_NAME)
BRL_OBJS =
$(BRL_FILE): brl.o
	$(LD) $(BRL_LFLAGS) -soname $(BRL_NAME) -o $(BRL_FILE) brl.o $(BRL_OBJS) -lc

braille-driver: $(BRL_FILE)

SPK_CFLAGS = $(LIB_CFLAGS) '-DSPKDRIVER="$(DRIVER_CODE)"'
SPK_LFLAGS = -shared
SPK_SO_NAME = $(LIB_SO_NAME)s
SPK_NAME = $(SPK_SO_NAME)$(DRIVER_CODE).so.$(LIB_VER)
SPK_FILE = $(LIBDIR)/$(SPK_NAME)
SPK_OBJS =
$(SPK_FILE): speech.o
	$(LD) $(SPK_LFLAGS) -soname $(SPK_NAME) -o $(SPK_FILE) speech.o $(SPK_OBJS) -lc

speech-driver: $(SPK_FILE)

ifeq ($(BRAILLE_MODELS),)
   BRAILLE_DESCRIPTION = $(DRIVER_NAME)
else
   BRAILLE_DESCRIPTION = $(DRIVER_NAME) [$(BRAILLE_MODELS)]
endif
brl-lib-name:
	echo -e "$(DRIVER_CODE)\t$(BRAILLE_DESCRIPTION)" >>$(BRLNAMES)

ifeq ($(SPEECH_MODELS),)
   SPEECH_DESCRIPTION = $(DRIVER_NAME)
else
   SPEECH_DESCRIPTION = $(DRIVER_NAME) [$(SPEECH_MODELS)]
endif
spk-lib-name:
	echo -e "$(DRIVER_CODE)\t$(SPEECH_DESCRIPTION)" >>$(SPKNAMES)

