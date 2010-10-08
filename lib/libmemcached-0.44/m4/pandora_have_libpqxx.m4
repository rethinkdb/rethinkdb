dnl  Copyright (C) 2010 Padraig O'Sullivan
dnl This file is free software; 
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBPQXX],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libpqxx
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libpqxx],
    [AS_HELP_STRING([--disable-libpqxx],
      [Build with libpqxx support @<:@default=on@:>@])],
    [ac_enable_libpqxx="$enableval"],
    [ac_enable_libpqxx="yes"])

  AS_IF([test "x$ac_enable_libpqxx" = "xyes"],[
    AC_LANG_PUSH([C++])
    AC_LIB_HAVE_LINKFLAGS(pqxx,,[
      #include <pqxx/pqxx>
    ],[
       pqxx::connection conn("dbname=test");
    ])
    AC_LANG_POP()
  ],[
    ac_cv_libpqxx="no"
  ])
  
  AM_CONDITIONAL(HAVE_LIBPQXX, [test "x${ac_cv_libpqxx}" = "xyes"])
  
])

AC_DEFUN([PANDORA_HAVE_LIBPQXX],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBPQXX])
])

AC_DEFUN([PANDORA_REQUIRE_LIBPQXX],[
  AC_REQUIRE([PANDORA_HAVE_LIBPQXX])
  AS_IF([test "x$ac_cv_libpqxx" = "xno"],[
      AC_MSG_ERROR([libpqxx is required for ${PACKAGE}])
  ])
])
