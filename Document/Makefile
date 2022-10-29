DOCUMENT_NAME = canute-controller

all: $(DOCUMENT_NAME).html

%.html: %.rst | command-help
	rst2html --config docutils.conf -- $< $@

command-help:
	./make-command-help

clean:
	-rm -f -- $(DOCUMENT_NAME).html *.hlp

