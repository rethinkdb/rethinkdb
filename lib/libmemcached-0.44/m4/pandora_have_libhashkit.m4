dnl  Copyright (C) 2010 NorthScale
dnl This file is free software; NorthScale
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBHASHKIT],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libhashkit
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libhashkit],
    [AS_HELP_STRING([--disable-libhashkit],
      [Build with libhashkit support @<:@default=on@:>@])],
    [ac_enable_libhashkit="$enableval"],
    [ac_enable_libhashkit="yes"])

  AS_IF([test "x$ac_enable_libhashkit" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(hashkit,,[
      #include <libhashkit/hashkit.h>
    ],[
      hashkit_st foo;
      hashkit_st *kit = hashkit_create(&foo);
      hashkit_free(kit);
    ])
  ],[
    ac_cv_libhashkit="no"
  ])

  AM_CONDITIONAL(HAVE_LIBHASHKIT, [test "x${ac_cv_libhashkit}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBHASHKIT],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBHASHKIT])
])

AC_DEFUN([PANDORA_REQUIRE_LIBHASHKIT],[
  AC_REQUIRE([PANDORA_HAVE_LIBHASHKIT])
  AS_IF([test x$ac_cv_libhashkit = xno],
      AC_MSG_ERROR([libhashkit is required for ${PACKAGE}]))
])
