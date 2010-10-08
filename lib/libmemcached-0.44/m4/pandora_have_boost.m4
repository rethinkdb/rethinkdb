dnl Copyright (C) 2010 Monty Taylor
dnl This file is free software; Monty Taylor
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_BOOST],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for boost
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([boost],
    [AS_HELP_STRING([--disable-boost],
      [Build with boost support @<:@default=on@:>@])],
    [ac_enable_boost="$enableval"],
    [ac_enable_boost="yes"])

  AS_IF([test "x$ac_enable_boost" = "xyes"],[
    dnl link against libc because we're just looking for headers here
    AC_LANG_PUSH(C++)
    AC_LIB_HAVE_LINKFLAGS(c,,[
      #include <boost/pool/pool.hpp>
    ],[
      boost::pool<> test_pool(1);
    ])
    AC_LANG_POP()
  ],[
    ac_cv_boost="no"
  ])
  

  AS_IF([test "x$1" != "x"],[
    AC_CACHE_CHECK([if boost is recent enough],
      [pandora_cv_recent_boost],[
      pandora_need_boost_version=`echo "$1" | perl -nle '/(\d+)\.(\d+)/; printf "%d%0.3d00", $[]1, $[]2 ;'`
      AS_IF([test "x${pandora_need_boost_version}" = "x000000"],[
        pandora_cv_recent_boost=yes
      ],[
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <boost/version.hpp>

#if BOOST_VERSION < ${pandora_need_boost_version}
# error boost too old!
#endif
          ]],[[]])
        ],[
          pandora_cv_recent_boost=yes
        ],[
          pandora_cv_recent_boost=no
        ])
      ])
    ])
    AS_IF([test "x${pandora_cv_recent_boost}" = "xno"],[
      ac_cv_boost=no
    ])
  ])
      

  AM_CONDITIONAL(HAVE_BOOST, [test "x${ac_cv_boost}" = "xyes"])
  
])

AC_DEFUN([PANDORA_HAVE_BOOST],[
  _PANDORA_SEARCH_BOOST($1)
])

AC_DEFUN([PANDORA_REQUIRE_BOOST],[
  PANDORA_HAVE_BOOST($1)
  AS_IF([test x$ac_cv_boost = xno],
      AC_MSG_ERROR([boost is required for ${PACKAGE}]))
])

