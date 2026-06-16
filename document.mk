###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2026 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.app/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

LOCALE = C
SETLOCALE = LC_ALL=$(LOCALE)

SPHINX_CONFDIR = $(SRC_TOP)Documents/Sphinx

all: all-yes
all-yes: txt html
all-no:
	@echo sphinx-build is not installed - document will not be made

txt: $(DOCUMENT_NAME).txt
html: html.made

$(DOCUMENT_NAME).txt: $(SRC_DIR)/$(DOCUMENT_NAME).rst
	$(SETLOCALE) sphinx-build -Q -b text -c $(SPHINX_CONFDIR) -D master_doc=$(DOCUMENT_NAME) $(SRC_DIR) _build/text
	cp _build/text/$(DOCUMENT_NAME).txt $@

html.made: $(SRC_DIR)/$(DOCUMENT_NAME).rst
	$(SETLOCALE) sphinx-build -Q -b singlehtml -c $(SPHINX_CONFDIR) -D master_doc=$(DOCUMENT_NAME) $(SRC_DIR) _build/html
	cp _build/html/$(DOCUMENT_NAME).html $(DOCUMENT_NAME).html
	touch $@

clean::
	-rm -f -- *.txt *.html *.made
	-rm -f -r _build
