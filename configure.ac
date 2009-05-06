# Init and options
AC_INIT([aweather], [0.1], [spenceal@rose-hulman.edu])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign])
AC_CONFIG_HEADERS([config.h])

# Check for required programs
AC_PROG_CC
PKG_PROG_PKG_CONFIG

# Check for required packages
PKG_CHECK_MODULES(GTK,  gtk+-2.0 gtkglext-1.0)
PKG_CHECK_MODULES(CURL, libcurl)

# Define odd RSL install location
AC_SUBST(RSL_CFLAGS, "-I/usr/local/trmm/GVBOX/include/")
AC_SUBST(RSL_LIBS,   "-L/usr/local/trmm/GVBOX/lib/ -lrsl")

# Output 
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT
