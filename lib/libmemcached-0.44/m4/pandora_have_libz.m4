dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for libz
#--------------------------------------------------------------------


AC_DEFUN([_PANDORA_SEARCH_LIBZ],[
  AC_REQUIRE([AC_LIB_PREFIX])

  AC_LIB_HAVE_LINKFLAGS(z,,
  [
    #include <zlib.h>
  ],[
    crc32(0, Z_NULL, 0);
  ])

  AM_CONDITIONAL(HAVE_LIBZ, [test "x${ac_cv_libz}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBZ],[

  AC_ARG_ENABLE([libz],
    [AS_HELP_STRING([--disable-libz],
      [Build with libz support @<:@default=on@:>@])],
    [ac_enable_libz="$enableval"],
    [ac_enable_libz="yes"])

  _PANDORA_SEARCH_LIBZ
])


AC_DEFUN([PANDORA_HAVE_LIBZ],[
  AC_REQUIRE([_PANDORA_HAVE_LIBZ])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBZ],[
  ac_enable_libz="yes"
  _PANDORA_SEARCH_LIBZ

  AS_IF([test x$ac_cv_libz = xno],[
    AC_MSG_ERROR([libz is required for ${PACKAGE}. On Debian this can be found in zlib1g-dev. On RedHat this can be found in zlib-devel.])
  ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBZ],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBZ])
])
