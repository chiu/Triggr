#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([triggr],[0.3],[M.Kursa@icm.edu.pl])
m4_include([ax_pthread.m4])
AC_CONFIG_SRCDIR([src/code.c])

# Checks for programs.
AC_MSG_CHECKING(for usable R installation)
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
  AC_MSG_ERROR([Cannot call R; be sure that your installation is correct.])
fi
AC_MSG_RESULT([yes])

RBIN="${R_HOME}/bin/R"
CC=`"${RBIN}" CMD config CC`;
CFLAGS=`"${RBIN}" CMD config CFLAGS`
AC_SUBST(R_HOME)
AC_PROG_CC


# Checks for libraries.

AX_PTHREAD(,[AC_MSG_ERROR([Cannot use pthreads.])])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"


AC_CHECK_LIB([ev],[ev_run],,AC_MSG_ERROR([Cannot find libev! Please install.]))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h netinet/in.h netdb.h stdlib.h string.h sys/socket.h unistd.h errno.h])
AC_CHECK_HEADER([ev.h],,AC_MSG_ERROR([Cannot find libev! Please install.]))

AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT

