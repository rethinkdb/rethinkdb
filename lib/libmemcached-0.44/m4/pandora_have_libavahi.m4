dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBAVAHI],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libavahi
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libavahi],
    [AS_HELP_STRING([--disable-libavahi],
      [Build with libavahi support @<:@default=on@:>@])],
    [ac_enable_libavahi="$enableval"],
    [ac_enable_libavahi="yes"])

  AS_IF([test "x$ac_enable_libavahi" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(avahi-client,avahi-common,[
      #include <avahi-client/client.h>
      #include <avahi-common/simple-watch.h>
    ],[
      AvahiSimplePoll *simple_poll= avahi_simple_poll_new();
    ])
  ],[
    ac_cv_libavahi="no"
  ])

  AM_CONDITIONAL(HAVE_LIBAVAHI, [test "x${ac_cv_libavahi}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBAVAHI],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBAVAHI])
])

AC_DEFUN([PANDORA_REQUIRE_LIBAVAHI],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBAVAHI])
  AS_IF([test "x${ac_cv_libavahi}" = "xno"],
    AC_MSG_ERROR([libavahi is required for ${PACKAGE}]))
])
