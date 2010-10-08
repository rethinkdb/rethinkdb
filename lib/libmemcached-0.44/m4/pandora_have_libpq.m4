dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBPQ],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libpq
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libpq],
    [AS_HELP_STRING([--disable-libpq],
      [Build with libpq support @<:@default=on@:>@])],
    [ac_enable_libpq="$enableval"],
    [ac_enable_libpq="yes"])

  AS_IF([test "x$ac_enable_libpq" = "xyes"],[
    AC_CHECK_HEADERS([libpq-fe.h])
    AC_LIB_HAVE_LINKFLAGS(pq,,[
      #ifdef HAVE_LIBPQ_FE_H
      # include <libpq-fe.h>
      #else
      # include <postgresql/libpq-fe.h>
      #endif
    ], [
      PGconn *conn;
      conn = PQconnectdb(NULL);
    ])
  ],[
    ac_cv_libpq="no"
  ])
  
  AM_CONDITIONAL(HAVE_LIBPQ, [test "x${ac_cv_libpq}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBPQ],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBPQ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBPQ],[
  AC_REQUIRE([PANDORA_HAVE_LIBPQ])
  AS_IF([test "x${ac_cv_libpq}" = "xno"],
    AC_MSG_ERROR([libpq is required for ${PACKAGE}]))
])
