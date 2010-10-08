dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for libpcre
#--------------------------------------------------------------------


AC_DEFUN([_PANDORA_SEARCH_LIBPCRE],[
  AC_REQUIRE([AC_LIB_PREFIX])

  AC_LIB_HAVE_LINKFLAGS(pcre,,
  [#include <pcre.h>],
  [
    pcre *re= NULL;
    pcre_version();
  ])
  AS_IF([test "x$ac_cv_libpcre" = "xno"],
  [
    unset ac_cv_libpcre
    unset HAVE_LIBPCRE
    unset LIBPCRE
    unset LIBPCRE_PREFIX
    unset LTLIBPCRE
    AC_LIB_HAVE_LINKFLAGS(pcre,,
    [#include <pcre/pcre.h>],
    [
      pcre *re= NULL;
      pcre_version();
    ])
    AS_IF([test "x$ac_cv_libpcre" = "xyes"], [
      ac_cv_pcre_location="<pcre/pcre.h>"
    ])
  ],[
    ac_cv_pcre_location="<pcre.h>"
  ])

  AM_CONDITIONAL(HAVE_LIBPCRE, [test "x${ac_cv_libpcre}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBPCRE],[

  AC_ARG_ENABLE([libpcre],
    [AS_HELP_STRING([--disable-libpcre],
      [Build with libpcre support @<:@default=on@:>@])],
    [ac_enable_libpcre="$enableval"],
    [ac_enable_libpcre="yes"])

  _PANDORA_SEARCH_LIBPCRE
])


AC_DEFUN([PANDORA_HAVE_LIBPCRE],[
  AC_REQUIRE([_PANDORA_HAVE_LIBPCRE])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBPCRE],[
  ac_enable_libpcre="yes"
  _PANDORA_SEARCH_LIBPCRE

  AS_IF([test x$ac_cv_libpcre = xno],[
    AC_MSG_ERROR([libpcre is required for ${PACKAGE}. On Debian this can be found in libpcre3-dev. On RedHat this can be found in pcre-devel.])
  ],[
    AC_DEFINE_UNQUOTED(PCRE_HEADER,[${ac_cv_pcre_location}],
                       [Location of pcre header])
  ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBPCRE],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBPCRE])
])
