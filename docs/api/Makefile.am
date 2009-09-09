AM_CPPFLAGS=$(SOUP_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS)
GTKDOC_LIBS=$(SOUP_LIBS) $(GLIB_LIBS) $(GTK_LIBS) \
	$(top_srcdir)/src/aweather-aweather-*.o \
	$(top_srcdir)/src/gis/libgis*.o
IGNORE_HFILES= 
if HAVE_RSL
GTKDOC_LIBS += $(RSL_LIBS) $(top_srcdir)/src/plugins/*.o
else
IGNORE_HFILES += radar.h
endif
DOC_MODULE=aweather
DOC_SOURCE_DIR=$(top_srcdir)/src/
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml
MKDB_OPTIONS=--sgml-mode --output-format=xml
include $(top_srcdir)/gtk-doc.make
CLEANFILES += aweather-sections.txt
MAINTAINERCLEANFILES=\
	tmpl/* Makefile.in aweather-docs.sgml aweather-overrides.txt aweather.types aweather-scan.*
