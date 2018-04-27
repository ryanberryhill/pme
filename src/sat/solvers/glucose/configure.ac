AC_INIT([glucose], [4.1])
AC_PREREQ([2.69])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.14 foreign subdir-objects])
AC_CONFIG_MACRO_DIR([../../../../m4])

AC_CONFIG_FILES([Makefile])

: ${CXXFLAGS="-g -O3"}

AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
AC_LANG([C++])
AC_PROG_RANLIB

AC_PROG_CC

AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_DFLT([memcheck], [off])
AX_VALGRIND_CHECK

AC_OUTPUT
