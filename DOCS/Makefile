all: en_txt en_html ger_txt ger_html

clean:
	rm Manual.txt Manual-ger.txt Manual-HTML/* Manual-ger-HTML/*

en_txt: Manual.sgml
	sgml2txt Manual.sgml

en_html: Manual.sgml
	cd Manual-HTML; \
	sgml2html ../Manual.sgml

ger_txt: Manual.sgml
	sgml2txt Manual-ger.sgml

ger_html: Manual-ger.sgml
	cd Manual-ger-HTML; \
	sgml2html ../Manual-ger.sgml
