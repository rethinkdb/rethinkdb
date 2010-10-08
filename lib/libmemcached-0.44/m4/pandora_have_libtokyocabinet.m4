dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
 
dnl Provides support for finding libtokyocabinet.
dnl LIBTOKYOCABINET_CFLAGS will be set, in addition to LIBTOKYOCABINET and LTLIBTOKYOCABINET

AC_DEFUN([_PANDORA_SEARCH_LIBTOKYOCABINET],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libtokyocabinet
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libtokyocabinet],
    [AS_HELP_STRING([--disable-libtokyocabinet],
      [Build with libtokyocabinet support @<:@default=on@:>@])],
    [ac_enable_libtokyocabinet="$enableval"],
    [ac_enable_libtokyocabinet="yes"])

  AS_IF([test "x$ac_enable_libtokyocabinet" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(tokyocabinet,,[
#include <tcutil.h>
#include <tcadb.h>
    ],[
const char *test= tcversion;
bool ret= tcadboptimize(NULL, "params");
    ])
  ],[
    ac_cv_libtokyocabinet="no"
  ])

  AS_IF([test "${ac_cv_libtokyocabinet}" = "no" -a "${ac_enable_libtokyocabinet}" = "yes"],[

    PKG_CHECK_MODULES([LIBTOKYOCABINET], [libtokyocabinet >= 1.4.15], [
      ac_cv_libtokyocabinet=yes
      LTLIBTOKYOCABINET=${LIBTOKYOCABINET_LIBS}
      LIBTOKYOCABINET=${LIBTOKYOCABINET_LIBS}
    ],[test x = y])
  ])

  AM_CONDITIONAL(HAVE_LIBTOKYOCABINET, [test "${ac_cv_libtokyocabinet}" = "yes"])
])

AC_DEFUN([PANDORA_HAVE_LIBTOKYOCABINET],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBTOKYOCABINET])
])

AC_DEFUN([PANDORA_REQUIRE_LIBTOKYOCABINET],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBTOKYOCABINET])
  AS_IF([test "x${ac_cv_libtokyocabinet}" = "xno"],
    AC_MSG_ERROR([libtokyocabinet is required for ${PACKAGE}. On Debian systems this is found in libtokyocabinet-dev. On RedHat, in tokyocabinet-devel.]))
])
