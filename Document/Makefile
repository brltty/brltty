DOCUMENT_NAME = software

all: $(DOCUMENT_NAME).html

%.html: %.rst | command-help
	rst2html --config docutils.conf -- $< $@

command-help:
	./make-command-help

clean:
	-rm -f -- $(DOCUMENT_NAME).html *.hlp

