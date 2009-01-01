###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2009 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

LOCALE = C
SETLOCALE = LC_ALL=$(LOCALE)

all: txt html
txt: $(DOCUMENT_NAME).txt
html: html.made

$(DOCUMENT_NAME).txt: $(SRC_DIR)/$(DOCUMENT_NAME).sgml
	$(SETLOCALE) linuxdoc -B txt -l $(DOCUMENT_LANGUAGE) -c latin $<
	sed -e 's/\x1B\[[0-9][0-9]*m//g' -i $@

html.made: $(SRC_DIR)/$(DOCUMENT_NAME).sgml
	$(SETLOCALE) linuxdoc -B html -l $(DOCUMENT_LANGUAGE) -c ascii $<
	touch $@

clean::
	-rm -f -- *.txt *.html *.made

