###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

SRC_TOP = ../
SRC_DIR = .
# VPATH = .

BLD_TOP = ../
BLD_DIR = .


include $(SRC_TOP)common.mk

###############################################################################
# BRLTTY - A background process providing access to the Linux console (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
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

include $(BLD_TOP)/config.mk
ABS_SRC_DIR = /users/dave/brltty/brltty-3.2/Documents

programs: english german

english: en_txt en_html

en_txt: Manual.txt

Manual.txt: $(SRC_DIR)/Manual.sgml
	sgml2txt -l english -c latin $(SRC_DIR)/Manual.sgml

en_html: $(ABS_SRC_DIR)/Manual.sgml
	$(INSTALL_DIRECTORY) Manual-HTML
	cd Manual-HTML && sgml2html -l english -c ascii $(ABS_SRC_DIR)/Manual.sgml

german: de_txt de_html

de_txt: Manual-ger.txt

Manual-ger.txt: $(SRC_DIR)/Manual-ger.sgml
	sgml2txt -l german -c latin $(SRC_DIR)/Manual-ger.sgml

de_html: $(ABS_SRC_DIR)/Manual-ger.sgml
	$(INSTALL_DIRECTORY) Manual-ger-HTML
	cd Manual-ger-HTML && sgml2html -l german -c ascii $(ABS_SRC_DIR)/Manual-ger.sgml

clean::
	-rm -f Manual.txt Manual-HTML/*
	-rm -f Manual-ger.txt Manual-ger-HTML/*

