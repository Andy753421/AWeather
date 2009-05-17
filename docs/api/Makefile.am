AM_CPPFLAGS=$(GTK_CFLAGS)
GTKDOC_LIBS=$(GTK_LIBS) $(RSL_LIBS) \
	../../src/aweather-aweather-*.o \
	../../src/aweather-plugin-*.o \
	../../src/aweather-location.o \
	../../src/aweather-data.o
DOC_MODULE=aweather
DOC_SOURCE_DIR=../../src/
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml
MKDB_OPTIONS=--sgml-mode --output-format=xml
MAINTAINERCLEANFILES= \
	Makefile.in aweather-docs.sgml aweather-overrides.txt aweather.types 
include $(top_srcdir)/gtk-doc.make
