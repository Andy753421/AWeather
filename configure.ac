# Init and options
AC_INIT([aweather], [0.6.1], [andy753421@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
#AM_SILENT_RULES([yes])

# Check for required programs
AC_PROG_CC
LT_PROG_RC
AC_PROG_LIBTOOL
DOLT
PKG_PROG_PKG_CONFIG

# Check for required packages
PKG_CHECK_MODULES(GLIB,  glib-2.0)
PKG_CHECK_MODULES(GRITS, grits >= 0.6)

# Define odd RSL install location
AC_CHECK_LIB(rsl, RSL_wsr88d_to_radar, RSL_LIBS="-lrsl")
AM_CONDITIONAL(HAVE_RSL, test "$RSL_LIBS" != "")
AC_SUBST(RSL_LIBS)

# Test for Windows vs. Unix
case "${host}" in
	*mingw32*) WIN32="yes" ;;
	*)         WIN32="no"  ;;
esac
if test "$WIN32" = yes; then
	ac_default_prefix="/"
fi
AM_CONDITIONAL([WIN32], test "$WIN32" = "yes")

# Output 
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/resource.rc
	src/plugins/Makefile
	data/Makefile
	data/main.ui
	docs/Makefile
])
AC_OUTPUT

# Print status messages
if test "$RSL_LIBS" = ""; then
	echo " ** Warning, RSL not found, radar plugin disabled"
fi
