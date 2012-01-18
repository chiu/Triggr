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
CPPFLAGS=`"${RBIN}" CMD config CPPFLAGS`
AC_SUBST(R_HOME)
AC_PROG_CC


# Checks for libraries
# pthreads
AX_PTHREAD(,[AC_MSG_ERROR([Cannot use pthreads.])])
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"


# libev
LIBEV_CFLAGS=""
LIBEV_LIBS=""


AC_CHECK_HEADERS([ev.h],[
	AC_CHECK_LIB([ev],[ev_time],[
		LIBEV_IN="<ev.h>"
		LIBEV_CFLAGS=""
		LIBEV_LIBS="-lev"
	],[
		AC_MSG_ERROR([libev headers found, but the library is not usable!])
	])
],[
		LIBEV_IN="<libev/ev.h>"
		AC_CHECK_HEADERS([libev/ev.h],[
			AC_CHECK_LIB([ev], [ev_time], [
				LIBEV_CFLAGS=""
				LIBEV_LIBS="-lev"
			],[
				AC_MSG_ERROR([older-style libev headers found, but the library is not usable!])
			])
		],[
			AC_MSG_ERROR([libev not found! Please install, or set ])
		])
])

AC_SUBST(LIBEV_IN)
LIBS="$LIBEV_LIBS $LIBS"
CFLAGS="$CFLAGS $LIBEV_CFLAGS"

#Checks for header files.
AC_CHECK_HEADERS([fcntl.h netinet/in.h netdb.h stdlib.h  sys/socket.h unistd.h errno.h])

AC_CONFIG_FILES([src/Makevars src/libev.h])
AC_OUTPUT

