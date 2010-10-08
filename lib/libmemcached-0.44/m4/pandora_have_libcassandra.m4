dnl  Copyright (C) 2010 Padraig O'Sullivan
dnl This file is free software; 
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBCASSANDRA],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libcassandra
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libcassandra],
    [AS_HELP_STRING([--disable-libcassandra],
      [Build with libcassandra support @<:@default=on@:>@])],
    [ac_enable_libcassandra="$enableval"],
    [ac_enable_libcassandra="yes"])

  AS_IF([test "x$ac_enable_libcassandra" = "xyes"],[
    AC_LANG_PUSH([C++])
    AC_LIB_HAVE_LINKFLAGS(cassandra,[thrift],[
      #include <libcassandra/cassandra_factory.h>
    ],[
       libcassandra::CassandraFactory fact("localhost", 9306);
    ])
    AC_LANG_POP()
  ],[
    ac_cv_libcassandra="no"
  ])
  
  AM_CONDITIONAL(HAVE_LIBCASSANDRA, [test "x${ac_cv_libcassandra}" = "xyes"])
  
])

AC_DEFUN([PANDORA_HAVE_LIBCASSANDRA],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBCASSANDRA])
])

AC_DEFUN([PANDORA_REQUIRE_LIBCASSANDRA],[
  AC_REQUIRE([PANDORA_HAVE_LIBCASSANDRA])
  AS_IF([test "x$ac_cv_libcassandra" = "xno"],[
      AC_MSG_ERROR([libcassandra is required for ${PACKAGE}])
  ])
])
