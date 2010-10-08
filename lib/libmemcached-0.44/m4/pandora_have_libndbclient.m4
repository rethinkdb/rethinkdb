dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl
dnl  Copyright (C) 2010 Monty Taylor
dnl  This file is free software; Sun Microsystems
dnl  gives unlimited permission to copy and/or distribute it,
dnl  with or without modifications, as long as this notice is preserved.
dnl

AC_DEFUN([_PANDORA_SEARCH_LIBNDBCLIENT],[

  AC_REQUIRE([AC_LIB_PREFIX])
  AC_REQUIRE([PANDORA_WITH_MYSQL])

  AC_ARG_ENABLE([libndbclient],
    [AS_HELP_STRING([--disable-libndbclient],
      [Build with libndbclient support @<:@default=on@:>@])],
    [ac_enable_libndbclient="$enableval"],
    [ac_enable_libndbclient="yes"])

  AC_ARG_WITH([libndbclient-prefix],
    [AS_HELP_STRING([--with-libndbclient-prefix],
      [search for libndbclient in DIR])],
    [ac_with_libndbclient=${withval}],
    [ac_with_libndbclient=${pandora_cv_mysql_base}])

  save_LIBS="${LIBS}"
  LIBS=""
  save_CPPFLAGS="${CPPFLAGS}"
  AS_IF([test "x${ac_with_libndbclient}" != "x"],[
    LIBS="-L${ac_with_libndbclient}/lib/mysql -L${ac_with_libndbclient}/lib"
    AS_IF([test "$GCC" = "yes"],[
      ndb_include_prefix="-isystem "
    ],[
      ndb_include_prefix="-I"
    ])
    CPPFLAGS="${CPPFLAGS} ${ndb_include_prefix}${ac_with_libndbclient}/include ${ndb_include_prefix}${ac_with_libndbclient}/include/mysql ${ndb_include_prefix}${ac_with_libndbclient}/include/mysql/storage/ndb ${ndb_include_prefix}${ac_with_libndbclient}/include/mysql/storage/ndb/ndbapi ${ndb_include_prefix}${ac_with_libndbclient}/include/mysql/storage/ndb/mgmapi"
  ])
  LIBS="${LIBS} -lndbclient -lmysqlclient_r"

  AC_CACHE_CHECK([if NdbApi works],[ac_cv_libndbclient],[
    AC_LANG_PUSH(C++)
    AC_LINK_IFELSE([
      AC_LANG_PROGRAM([[
#include <NdbApi.hpp>
      ]],[[
Ndb *ndb;
ndb_init();
      ]])
    ],[
      ac_cv_libndbclient=yes
    ],[
      ac_cv_libndbclient=no
    ])
  ])
  AC_LANG_POP()

  LIBNDBCLIENT="${LIBS}"
  LTLIBNDBCLIENT="${LIBS}"
  AC_SUBST([LIBNDBCLIENT])
  AC_SUBST([LTLIBNDBCLIENT])

  AS_IF([test "x${ac_cv_libndbclient}" = "xno"],[
    CPPFLAGS="${save_CPPFLAGS}"
  ])    
  LIBS="${save_LIBS}"
  
  AM_CONDITIONAL(HAVE_LIBNDBCLIENT, [test "x${ac_cv_libndbclient}" = "xyes"])
])
  
AC_DEFUN([PANDORA_HAVE_LIBNDBCLIENT],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBNDBCLIENT])
])

AC_DEFUN([PANDORA_REQUIRE_LIBNDBCLIENT],[
  AC_REQUIRE([PANDORA_HAVE_LIBNDBCLIENT])
  AS_IF([test "x${ac_cv_libndbclient}" = "xno"],
      AC_MSG_ERROR([libndbclient is required for ${PACKAGE}]))
])

