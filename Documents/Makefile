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

all: faq en ger

faq: FAQ.txt

FAQ.txt: FAQ.html
	lynx -dump -width=79 $< >$@

en: en_txt en_html

en_txt: Manual.txt

en_chk: Manual.txt
	ul -t dumb Manual.txt | spell -p en.dict -w 'é'

Manual.txt: Manual.sgml
	sgml2txt -l english -c latin Manual.sgml

en_html: Manual.sgml
	cd Manual-HTML; \
	sgml2html -l english -c ascii ../Manual.sgml

ger: ger_txt ger_html

ger_txt: Manual.sgml
	sgml2txt -l german -c latin Manual-ger.sgml

ger_html: Manual-ger.sgml
	cd Manual-ger-HTML; \
	sgml2html -l german -c ascii ../Manual-ger.sgml

clean:
	rm -f Manual.txt Manual-ger.txt Manual-HTML/* Manual-ger-HTML/*

