dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBDRIZZLE],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libdrizzle
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libdrizzle],
    [AS_HELP_STRING([--disable-libdrizzle],
      [Build with libdrizzle support @<:@default=on@:>@])],
    [ac_enable_libdrizzle="$enableval"],
    [ac_enable_libdrizzle="yes"])

  AS_IF([test "x$ac_enable_libdrizzle" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(drizzle,,[
      #include <libdrizzle/drizzle_client.h>
    ],[
      drizzle_st drizzle;
      drizzle_version();
    ])
  ],[
    ac_cv_libdrizzle="no"
  ])
  
  AM_CONDITIONAL(HAVE_LIBDRIZZLE, [test "x${ac_cv_libdrizzle}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBDRIZZLE],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBDRIZZLE])
])

AC_DEFUN([PANDORA_REQUIRE_LIBDRIZZLE],[
  AC_REQUIRE([PANDORA_HAVE_LIBDRIZZLE])
  AS_IF([test "x${ac_cv_libdrizzle}" = "xno"],[
    AC_MSG_ERROR([libdrizzle is required for ${PACKAGE}])
  ],[
    dnl We need at least 0.8 on Solaris non-sparc
    AS_IF([test "$target_cpu" != "sparc" -a "x${TARGET_SOLARIS}" = "xtrue"],[
      PANDORA_LIBDRIZZLE_RECENT
    ])
  ])
])

AC_DEFUN([PANDORA_LIBDRIZZLE_RECENT],[
  AC_CACHE_CHECK([if libdrizzle is recent enough],
    [pandora_cv_libdrizzle_recent],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <libdrizzle/drizzle.h>
drizzle_con_options_t foo= DRIZZLE_CON_EXPERIMENTAL;
    ]])],
    [pandora_cv_libdrizzle_recent=yes],
    [pandora_cv_libdrizzle_recent=no])])
  AS_IF([test "$pandora_cv_libdrizzle_recent" = "no"],[
    AC_MSG_ERROR([Your version of libdrizzle is too old. ${PACKAGE} requires at least version 0.8])
  ])
])
