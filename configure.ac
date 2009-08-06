# Init and options
AC_INIT([aweather], [0.2.0], [spenceal@rose-hulman.edu])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

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
#AC_SUBST(RSL_CFLAGS, "-I/usr/local/trmm/GVBOX/include/")
#AC_SUBST(RSL_LIBS,   "-L/usr/local/trmm/GVBOX/lib/ -lrsl")
AC_SUBST(RSL_CFLAGS, "")
AC_SUBST(RSL_LIBS,   "-lrsl")

# Output 
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/gis/Makefile
	src/plugins/Makefile
	data/Makefile
	docs/Makefile
	docs/api/Makefile
])
AC_OUTPUT
