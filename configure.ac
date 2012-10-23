#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([kinect-bleeper], [0.0], [http://github.com/coolparadox/kinect-bleeper/issues], [], [http://coolparadox.github.com/kinect-bleeper])
AM_INIT_AUTOMAKE([gnu])
AC_CONFIG_SRCDIR([src/kinect_bleeper.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([freenect], [freenect_init], [], [AC_MSG_ERROR([this tool requires libfreenect.])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER([argp.h],
	[AC_DEFINE([HAVE_ARGP_H], [1], [Define to 1 if you have <argp.h>.])],
	[AC_MSG_ERROR([missing argp.h])])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Add configure option for enabling debug code.
AC_ARG_ENABLE(
	debug,
	[AS_HELP_STRING(
		[--enable-debug], 
		[enable debug code @<:@default=no@:>@])],
	[case "$enableval" in
		yes|no) debug=$enableval ;;
		*) AC_MSG_ERROR(bad value "$enableval" for --enable-debug) ;;
	esac],
	[debug=no])
AM_CONDITIONAL(DEBUG, test "x$debug" = xyes)
AH_TEMPLATE([DEBUG], [Define to enable debug code.])
test "x$debug" = xyes && AC_DEFINE([DEBUG], [])
AC_MSG_RESULT([debug code enablement... $debug])

# Process pattern replacements.
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

