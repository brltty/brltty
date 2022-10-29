all: canute-controller.html

%.html: %.rst | command-help
	rst2html --config docutils.conf -- $< $@

command-help:
	./make-command-help

clean:
	-rm -f -- *.html *.hlp

