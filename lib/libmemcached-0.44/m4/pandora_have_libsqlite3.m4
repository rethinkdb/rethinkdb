dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBSQLITE3],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libsqlite3
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libsqlite3],
    [AS_HELP_STRING([--disable-libsqlite3],
      [Build with libsqlite3 support @<:@default=on@:>@])],
    [ac_enable_libsqlite3="$enableval"],
    [ac_enable_libsqlite3="yes"])

  AS_IF([test "x$ac_enable_libsqlite3" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(sqlite3,,[
      #include <stdio.h>
      #include <sqlite3.h>
    ],[
      sqlite3 *db;
      sqlite3_open(NULL, &db);
    ])
  ],[
    ac_cv_libsqlite3="no"
  ])

  AM_CONDITIONAL(HAVE_LIBSQLITE3, [test "x${ac_cv_libsqlite3}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBSQLITE3],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBSQLITE3])
])

AC_DEFUN([PANDORA_REQUIRE_LIBSQLITE3],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBSQLITE3])
  AS_IF([test "x${ac_cv_libsqlite3}" = "xno"],
    AC_MSG_ERROR([libsqlite3 is required for ${PACKAGE}]))
])
