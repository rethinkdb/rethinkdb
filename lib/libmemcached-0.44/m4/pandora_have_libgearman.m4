dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBGEARMAN],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libgearman
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libgearman],
    [AS_HELP_STRING([--disable-libgearman],
      [Build with libgearman support @<:@default=on@:>@])],
    [ac_enable_libgearman="$enableval"],
    [ac_enable_libgearman="yes"])

  AS_IF([test "x$ac_enable_libgearman" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(gearman,,[
      #include <libgearman/gearman.h>
    ],[
      gearman_client_st gearman_client;
      gearman_client_context(&gearman_client);
    ])
  ],[
    ac_cv_libgearman="no"
  ])

  AM_CONDITIONAL(HAVE_LIBGEARMAN, [test "x${ac_cv_libgearman}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBGEARMAN],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBGEARMAN])
])

AC_DEFUN([PANDORA_REQUIRE_LIBGEARMAN],[
  AC_REQUIRE([PANDORA_HAVE_LIBGEARMAN])
  AS_IF([test "x${ac_cv_libgearman}" = "xno"],
      AC_MSG_ERROR([At least version 0.10 of libgearman is required for ${PACKAGE}]))
])
