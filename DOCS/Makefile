###############################################################################
# BrlTty - A daemon providing access to the Linux console (when in text
#          mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2001 by The BrlTty Team. All rights reserved.
#
# BrlTty comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

all: en_txt en_html ger_txt ger_html

clean:
	rm Manual.txt Manual-ger.txt Manual-HTML/* Manual-ger-HTML/*

en_txt: Manual.sgml
	sgml2txt -f -l Manual.sgml

en_html: Manual.sgml
	cd Manual-HTML; \
	sgml2html -l ../Manual.sgml

ger_txt: Manual.sgml
	sgml2txt -f -l Manual-ger.sgml
#	sgml2txt -f -l de -c latin Manual-ger.sgml

ger_html: Manual-ger.sgml
	cd Manual-ger-HTML; \
	sgml2html -l ../Manual-ger.sgml
#	sgml2html -l de -c latin ../Manual-ger.sgml
