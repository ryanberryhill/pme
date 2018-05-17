AC_INIT([pme], [0.1.1], [ryan@berryhill.xyz])
AC_PREREQ([2.69])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.14 foreign subdir-objects -Wall -Werror])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([src/sat/solvers/glucose src/sat/solvers/minisat])

AC_PROG_CXX
# Enable C++11
AX_CXX_COMPILE_STDCXX_11

AC_PROG_CC
# Enable C99
AC_PROG_CC_C99

AC_PROG_RANLIB
AM_PROG_AR

# To enable valgrind checks
AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_DFLT([memcheck], [on])
AX_VALGRIND_CHECK

AC_OUTPUT
