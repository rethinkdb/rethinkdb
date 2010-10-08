dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBBDB],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for bekerely db
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libbdb],
    [AS_HELP_STRING([--disable-libbdb],
      [Build with libbdb support @<:@default=on@:>@])],
    [ac_enable_libbdb="$enableval"],
    [ac_enable_libbdb="yes"])

  AS_IF([test "x$ac_enable_libbdb" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(db,,[
      #include <db.h>
    ],[
      const char *test= DB_VERSION_STRING;
    ])
  ],[
    ac_cv_libbdb="no"
  ])

  AM_CONDITIONAL(HAVE_LIBBDB, [test "x${ac_cv_libbdb}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBBDB],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBBDB])
])

AC_DEFUN([PANDORA_REQUIRE_LIBBDB],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBBDB])
  AS_IF([test "x${ac_cv_libbdb}" = "xno"],
    AC_MSG_ERROR([libbdb is required for ${PACKAGE}]))
])
