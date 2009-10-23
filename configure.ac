# Init and options
AC_INIT([aweather], [0.3-p0], [spenceal@rose-hulman.edu])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
#AM_SILENT_RULES([yes])

# Check for required programs
AC_PROG_CC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG
GTK_DOC_CHECK(1.9)

# Check for required packages
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.14 gobject-2.0 gthread-2.0 gmodule-export-2.0)
PKG_CHECK_MODULES(GTK,  gtk+-2.0 >= 2.16 gtkglext-1.0)
PKG_CHECK_MODULES(SOUP, libsoup-2.4 >= 2.26)

# Define odd RSL install location
AC_CHECK_LIB(rsl, RSL_wsr88d_to_radar, RLS_LIBS="-lrsl")
AM_CONDITIONAL(HAVE_RSL, test "$RSL_LIBS" != "")

# Output 
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/gis/Makefile
	src/gis/libgis.pc
	src/plugins/Makefile
	data/Makefile
	docs/Makefile
	docs/api/Makefile
])
AC_OUTPUT

# Print status messages
if test "$RSL_LIBS" = ""; then
	echo " ** Warning, RSL not found, radar plugin disabled"
fi
