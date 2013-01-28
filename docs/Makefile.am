man1_MANS = aweather.1 wsr88ddec.1
html_DATA = aweather.html userguide.html

EXTRA_DIST = $(man1_MANS) $(html_DATA) \
	     aweather.ad userguide.ad wsr88ddec.ad

%.html: %.ad
	asciidoc $<

%.1: %.ad
	a2x -f manpage $<

test: $(man1_MANS)
	nroff -man $(man1_MANS)

maintainer-clean-local:
	rm -f Makefile.in
