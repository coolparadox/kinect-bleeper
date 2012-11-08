#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([kinect-bleeper], [1.0], [http://github.com/coolparadox/kinect-bleeper/issues], [], [http://github.com/coolparadox/kinect-bleeper])
AM_INIT_AUTOMAKE([gnu])
AC_CONFIG_SRCDIR([src/bleeper.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_CHECK_PROG([has_pkg_config], [pkg-config], [yes], [no])

# Checks for libraries.
AC_CHECK_LIB([freenect], [freenect_init], [], [AC_MSG_ERROR([this tool requires libfreenect.])])
AC_CHECK_LIB([ao], [ao_initialize], [], [AC_MSG_ERROR([this tool requires libao.])])
AC_CHECK_LIB([gtk-3], [gtk_widget_new], [has_gtk=yes], [has_gtk=no])

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

# Add configure option for enabling a gui monitor.
AC_ARG_ENABLE(
	gtk,
	[AS_HELP_STRING(
		[--enable-gtk], 
		[enable a gui monitor using gtk-3])],
	[case "$enableval" in
		yes)
			test x$has_gtk = xyes || AC_MSG_ERROR([cannot honor --enable-gtk due to missing libgtk-3.])
			gtk=$enableval
			;;
		no) gtk=$enableval ;;
		*) AC_MSG_ERROR(bad value "$enableval" for --enable-gtk) ;;
	esac],
	[gtk=$has_gtk])
AM_CONDITIONAL(GTK, test "x$gtk" = xyes)
AH_TEMPLATE([GTK], [Define to enable gtk code.])
test "x$gtk" = xyes && AC_DEFINE([GTK], [])
AC_MSG_RESULT([build gtk-3 gui monitor... $gtk])

if test x$gtk = xyes ; then
	test x$has_pkg_config = xyes || AC_MSG_ERROR([cannot get gtk+-3.0 cflags due to missing pkg-config.])
	gtk_cflags=$( pkg-config --cflags gtk+-3.0 )
	gtk_libs=$( pkg-config --libs gtk+-3.0 )
	AC_SUBST([GTK_CFLAGS], [$gtk_cflags])
	AC_SUBST([GTK_LDFLAGS], [$gtk_libs])
else
	AC_SUBST([GTK_CFLAGS], [""])
	AC_SUBST([GTK_LDFLAGS], [""])
fi

# Process pattern replacements.
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

