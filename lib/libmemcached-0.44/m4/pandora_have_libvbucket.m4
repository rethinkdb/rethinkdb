dnl  Copyright (C) 2010 NorthScale
dnl This file is free software; NorthScale
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBVBUCKET],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libvbucket
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libvbucket],
    [AS_HELP_STRING([--disable-libvbucket],
      [Build with libvbucket support @<:@default=on@:>@])],
    [ac_enable_libvbucket="$enableval"],
    [ac_enable_libvbucket="yes"])

  AS_IF([test "x$ac_enable_libvbucket" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(vbucket,,[
      #include <libvbucket/vbucket.h>
    ],[
      VBUCKET_CONFIG_HANDLE config = vbucket_config_parse_file(NULL);
    ])
  ],[
    ac_cv_libvbucket="no"
  ])

  AM_CONDITIONAL(HAVE_LIBVBUCKET, [test "x${ac_cv_libvbucket}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBVBUCKET],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBVBUCKET])
])

AC_DEFUN([PANDORA_REQUIRE_LIBVBUCKET],[
  AC_REQUIRE([PANDORA_HAVE_LIBVBUCKET])
  AS_IF([test x$ac_cv_libvbucket = xno],
      AC_MSG_ERROR([libvbucket is required for ${PACKAGE}]))
])
