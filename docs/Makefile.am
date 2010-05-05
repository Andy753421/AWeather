man1_MANS = aweather.1
html_DATA = aweather.html

%.html: %.ad
	asciidoc $<

%.1: %.ad
	a2x -f manpage $<

test: $(man1_MANS)
	nroff -man $(man1_MANS)

maintainer-clean-local:
	rm -f Makefile.in
