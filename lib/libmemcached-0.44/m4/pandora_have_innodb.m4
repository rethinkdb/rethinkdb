dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBINNODB],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libinnodb
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libinnodb],
    [AS_HELP_STRING([--disable-libinnodb],
      [Build with libinnodb support @<:@default=on@:>@])],
    [ac_enable_libinnodb="$enableval"],
    [ac_enable_libinnodb="yes"])

  AS_IF([test "x$ac_enable_libinnodb" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(innodb,,[
      #include <embedded_innodb-1.0/innodb.h>
    ],[
      ib_u64_t
      ib_api_version(void);
    ])
  ],[
    ac_cv_libinnodb="no"
  ])

  AM_CONDITIONAL(HAVE_LIBINNODB, [test "x${ac_cv_libinnodb}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBINNODB],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBINNODB])
])

AC_DEFUN([PANDORA_REQUIRE_LIBINNODB],[
  AC_REQUIRE([PANDORA_HAVE_LIBINNODB])
  AS_IF([test "x${ac_cv_libinnodb}" = "xno"],
      AC_MSG_ERROR([libinnodb is required for ${PACKAGE}]))
])
