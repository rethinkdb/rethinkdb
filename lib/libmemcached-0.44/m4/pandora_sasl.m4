dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_SASL],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for sasl
  dnl --------------------------------------------------------------------
  AC_ARG_ENABLE([sasl],
    [AS_HELP_STRING([--disable-sasl],
      [Build with sasl support @<:@default=on@:>@])],
    [ac_enable_sasl="$enableval"],
    [ac_enable_sasl="yes"])

  AS_IF([test "x$ac_enable_sasl" = "xyes"],
    [
      AC_LIB_HAVE_LINKFLAGS(sasl,,[
        #include <stdlib.h>
        #include <sasl/sasl.h>
      ],[
        sasl_server_init(NULL, NULL);
      ])

      AS_IF([test "x${ac_cv_libsasl}" != "xyes" ],
            [
              AC_LIB_HAVE_LINKFLAGS(sasl2,,[
                #include <stdlib.h>
                #include <sasl/sasl.h>
              ],[
                sasl_server_init(NULL, NULL);
              ])
              HAVE_LIBSASL="$HAVE_LIBSASL2"
              LIBSASL="$LIBSASL2"
              LIBSASL_PREFIX="$LIBSASL2_PREFIX"
	      LTLIBSASL="$LT_LIBSASL2"
            ])
    ])

  AS_IF([test "x${ac_cv_libsasl}" = "xyes" -o "x${ac_cv_libsasl2}" = "xyes"],
        [ac_cv_sasl=yes],
        [ac_cv_sasl=no])

  AM_CONDITIONAL(HAVE_LIBSASL, [test "x${ac_cv_libsasl}" = "xyes"])
  AM_CONDITIONAL(HAVE_LIBSASL2, [test "x${ac_cv_libsasl2}" = "xyes"])
  AM_CONDITIONAL(HAVE_SASL, [test "x${ac_cv_sasl}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_SASL],[
  AC_REQUIRE([_PANDORA_SEARCH_SASL])
])

AC_DEFUN([PANDORA_REQUIRE_SASL],[
  AC_REQUIRE([_PANDORA_SEARCH_SASL])
  AS_IF([test "x${ac_cv_sasl}" = "xno"],
    AC_MSG_ERROR([SASL (libsasl or libsasl2) is required for ${PACKAGE}]))
])

AC_DEFUN([_PANDORA_SEARCH_LIBSASL],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libsasl
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libsasl],
    [AS_HELP_STRING([--disable-libsasl],
      [Build with libsasl support @<:@default=on@:>@])],
    [ac_enable_libsasl="$enableval"],
    [ac_enable_libsasl="yes"])

  AS_IF([test "x$ac_enable_libsasl" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(sasl,,[
      #include <stdlib.h>
      #include <sasl/sasl.h>
    ],[
      sasl_server_init(NULL, NULL);
    ])
  ],[
    ac_cv_libsasl="no"
  ])

  AM_CONDITIONAL(HAVE_LIBSASL, [test "x${ac_cv_libsasl}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBSASL],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBSASL])
])

AC_DEFUN([PANDORA_REQUIRE_LIBSASL],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBSASL])
  AS_IF([test "x${ac_cv_libsasl}" = "xno"],
    AC_MSG_ERROR([libsasl is required for ${PACKAGE}]))
])

AC_DEFUN([_PANDORA_SEARCH_LIBSASL2],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libsasl2
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libsasl2],
    [AS_HELP_STRING([--disable-libsasl2],
      [Build with libsasl2 support @<:@default=on@:>@])],
    [ac_enable_libsasl2="$enableval"],
    [ac_enable_libsasl2="yes"])

  AS_IF([test "x$ac_enable_libsasl2" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(sasl2,,[
      #include <stdlib.h>
      #include <sasl2/sasl2.h>
    ],[
      sasl2_server_init(NULL, NULL);
    ])
  ],[
    ac_cv_libsasl2="no"
  ])

  AM_CONDITIONAL(HAVE_LIBSASL2, [test "x${ac_cv_libsasl2}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBSASL2],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBSASL2])
])

AC_DEFUN([PANDORA_REQUIRE_LIBSASL2],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBSASL2])
  AS_IF([test "x${ac_cv_libsasl2}" = "xno"],
    AC_MSG_ERROR([libsasl2 is required for ${PACKAGE}]))
])
