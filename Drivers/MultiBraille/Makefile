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
# Makefile for the Tieman B.V. MultiBraille driver
# $Id: Makefile,v 1.1 1996/09/25 00:26:28 nn201 Exp $
###############################################################################

DRIVER_CODE = mb
DRIVER_NAME = MultiBraille
BRAILLE_MODELS = MB125CR/MB145CR/MB185CR
HELP_DEPS = $(HELP_TEXT)

include setup.mk
include $(TOP_DIR)/driver.mk

braille.o:
	$(CC) $(BRL_CFLAGS) -c braille.c

