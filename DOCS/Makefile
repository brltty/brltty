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
