#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(SCPTestApp, 1.0.0, akashonnet@gmail.com)
AC_CONFIG_SRCDIR([scptest.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AM_PROG_LIBTOOL

#Set shared libraries
AC_DISABLE_STATIC
AC_ENABLE_SHARED

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([getpass strdup strerror strncasecmp])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AM_INIT_AUTOMAKE