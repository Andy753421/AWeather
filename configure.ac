# Init and options
AC_INIT([aweather], [0.3-p1], [spenceal@rose-hulman.edu])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
#AM_SILENT_RULES([yes])

# Check for required programs
AC_PROG_CC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# Check for required packages
PKG_CHECK_MODULES(GIS, libgis = $VERSION)

# Define odd RSL install location
AC_CHECK_LIB(rsl, RSL_wsr88d_to_radar, RSL_LIBS="-lrsl")
AM_CONDITIONAL(HAVE_RSL, test "$RSL_LIBS" != "")
AC_SUBST(RSL_LIBS)

# Output 
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/plugins/Makefile
	data/Makefile
	docs/Makefile
])
AC_OUTPUT

# Print status messages
if test "$RSL_LIBS" = ""; then
	echo " ** Warning, RSL not found, radar plugin disabled"
fi
