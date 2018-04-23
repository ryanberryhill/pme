AC_INIT([pme], [0.0.1], [ryan@berryhill.xyz])
AC_PREREQ([2.69])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.14 foreign subdir-objects -Wall -Werror])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([src/sat/solvers/glucose])

AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

AC_OUTPUT
