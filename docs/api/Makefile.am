AM_CPPFLAGS=$(GTK_CFLAGS)
GTKDOC_LIBS=$(GTK_LIBS) $(RSL_LIBS) $(CURL_LIBS) \
	../../src/aweather-gui.o \
	../../src/aweather-view.o \
	../../src/location.o \
	../../src/opengl.o \
	../../src/plugin-example.o \
	../../src/plugin-radar.o \
	../../src/plugin-ridge.o
DOC_MODULE=aweather
DOC_SOURCE_DIR=../../src/
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml
MKDB_OPTIONS=--sgml-mode --output-format=xml
include $(top_srcdir)/gtk-doc.make
maintainer-clean-local:
	rm -f Makefile.in aweather-docs.sgml aweather-overrides.txt aweather.types
	rm -rf html/ tmpl/ 
