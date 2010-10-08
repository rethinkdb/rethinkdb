dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
 
dnl Provides support for finding libxml2.
dnl LIBXML2_CFLAGS will be set, in addition to LIBXML2 and LTLIBXML2

AC_DEFUN([_PANDORA_SEARCH_LIBXML2],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libxml2
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libxml2],
    [AS_HELP_STRING([--disable-libxml2],
      [Build with libxml2 support @<:@default=on@:>@])],
    [ac_enable_libxml2="$enableval"],
    [ac_enable_libxml2="yes"])

  AS_IF([test "x$ac_enable_libxml2" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(xml2,,[
#include <libxml/xmlversion.h>
    ],[
const char *test= LIBXML_DOTTED_VERSION;
    ])
  ],[
    ac_cv_libxml2="no"
  ])

  AS_IF([test "${ac_cv_libxml2}" = "no" -a "${ac_enable_libxml2}" = "yes"],[

    PKG_CHECK_MODULES([LIBXML2], [libxml-2.0], [
      ac_cv_libxml2=yes
      LTLIBXML2=${LIBXML2_LIBS}
      LIBXML2=${LIBXML2_LIBS}
    ],[])
  ])

  AM_CONDITIONAL(HAVE_LIBXML2, [test "${ac_cv_libxml2}" = "yes"])
])

AC_DEFUN([PANDORA_HAVE_LIBXML2],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBXML2])
])

AC_DEFUN([PANDORA_REQUIRE_LIBXML2],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBXML2])
  AS_IF([test "x${ac_cv_libxml2}" = "xno"],
    AC_MSG_ERROR([libxml2 is required for ${PACKAGE}. On Debian systems this is found in libxml2-dev. On RedHat, libxml2-devel.]))
])
