dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl
dnl  Copyright (C) 2010 Monty Taylor
dnl  This file is free software; Sun Microsystems
dnl  gives unlimited permission to copy and/or distribute it,
dnl  with or without modifications, as long as this notice is preserved.
dnl

AC_DEFUN([PANDORA_WITH_MYSQL],[
  AC_ARG_WITH([mysql],
    [AS_HELP_STRING([--with-mysql=PATH],
        [path to mysql_config binary or mysql prefix dir])], 
      [with_mysql=$withval],
      [with_mysql=":"])

  dnl There are three possibilities:
  dnl   1) nothing is given: we will search for mysql_config in PATH
  dnl   2) the location of mysql_config is given: we'll use that to determine
  dnl   3) a directory argument is given: that will be mysql_base

     
  dnl option 1: nothing, we need to insert something into MYSQL_CONFIG
  AS_IF([test "x$with_mysql" = "x:"],[
    AC_CHECK_PROGS(MYSQL_CONFIG,[mysql_config])
  ],[
    MYSQL_CONFIG="${with_mysql}"
  ])

  AC_CACHE_CHECK([for MySQL Base Location],[pandora_cv_mysql_base],[

    dnl option 2: something in MYSQL_CONFIG now, use that to get a base dir
    AS_IF([test -f "${MYSQL_CONFIG}" -a -x "${MYSQL_CONFIG}"],[
      pandora_cv_mysql_base=$(dirname $(MYSQL_CONFIG --include | sed 's/-I//'))
    ],[
      dnl option 1: a directory
      AS_IF([test -d $with_mysql],[pandora_cv_mysql_base=$with_mysql],[
        pandora_cv_mysql_base="not found"
      ])
    ])
  ])
])

AC_DEFUN([_PANDORA_SEARCH_LIBMYSQLCLIENT],[
  AC_REQUIRE([AC_LIB_PREFIX])

  AC_ARG_ENABLE([libmysqlclient],
    [AS_HELP_STRING([--disable-libmysqlclient],
      [Build with libmysqlclient support @<:@default=on@:>@])],
    [ac_enable_libmysqlclient="$enableval"],
    [ac_enable_libmysqlclient="yes"])

  AS_IF([test "x$ac_enable_libmysqlclient" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(mysqlclient_r,,[
#include <mysql/mysql.h>
    ],[
MYSQL mysql;
  ])],[
    ac_cv_libmysqlclient_r="no"
  ])

  AM_CONDITIONAL(HAVE_LIBMYSQLCLIENT, [test "x${ac_cv_libmysqlclient_r}" = "xyes"])
  
AC_DEFUN([PANDORA_HAVE_LIBMYSQLCLIENT],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBMYSQLCLIENT])
])

AC_DEFUN([PANDORA_REQUIRE_LIBMYSQLCLIENT],[
  AC_REQUIRE([PANDORA_HAVE_LIBMYSQLCLIENT])
  AS_IF([test "x${ac_cv_libmysqlclient_r}" = "xno"],
      AC_MSG_ERROR([libmysqlclient_r is required for ${PACKAGE}]))
])

  AS_IF([test "x$MYSQL_CONFIG" = "xISDIR"],[
    IBASE="-I${with_mysql}"
    MYSQL_CONFIG="${with_mysql}/scripts/mysql_config"
    ADDIFLAGS="$IBASE/include "
    ADDIFLAGS="$ADDIFLAGS $IBASE/storage/ndb/include/ndbapi "
    ADDIFLAGS="$ADDIFLAGS $IBASE/storage/ndb/include/mgmapi "
    ADDIFLAGS="$ADDIFLAGS $IBASE/storage/ndb/include "
    LDFLAGS="-L${with_mysql}/storage/ndb/src/.libs -L${with_mysql}/libmysql_r/.libs/ -L${with_mysql}/mysys/.libs -L${with_mysql}/mysys -L${with_mysql}/strings/.libs -L${with_mysql}/strings "
  ],[
    IBASE=`$MYSQL_CONFIG --include`
    ADDIFLAGS=""
    # add regular MySQL C flags
    ADDCFLAGS=`$MYSQL_CONFIG --cflags` 
    # add NdbAPI specific C flags
    LDFLAGS="$LDFLAGS "`$MYSQL_CONFIG --libs_r | sed 's/-lmysqlclient_r//'`
    ])


    ADDIFLAGS="$ADDIFLAGS $IBASE/storage/ndb"
    ADDIFLAGS="$ADDIFLAGS $IBASE/storage/ndb/ndbapi"
    ADDIFLAGS="$ADDIFLAGS $IBASE/storage/ndb/mgmapi"
    ADDIFLAGS="$ADDIFLAGS $IBASE/ndb"
    ADDIFLAGS="$ADDIFLAGS $IBASE/ndb/ndbapi"
    ADDIFLAGS="$ADDIFLAGS $IBASE/ndb/mgmapi"
    ADDIFLAGS="$ADDIFLAGS $IBASE"

    CFLAGS="$CFLAGS $ADDCFLAGS $ADDIFLAGS"    
    CXXFLAGS="$CXXFLAGS $ADDCFLAGS $ADDIFLAGS" 
    MYSQL_INCLUDES="$IBASE $ADDIFLAGS"   

    
    dnl AC_CHECK_LIB([mysqlclient_r],[safe_mutex_init],,[AC_MSG_ERROR([Can't link against libmysqlclient_r])])
    dnl First test to see if we can run with only ndbclient
    AC_CHECK_LIB([ndbclient],[decimal_bin_size],,[dnl else
      LDFLAGS="$LDFLAGS -lmysys -ldbug"
      AC_CHECK_LIB([mysqlclient_r],[safe_mutex_init],,)
      AC_CHECK_LIB([ndbclient],[ndb_init],,[
        AC_MSG_ERROR([Can't link against libndbclient])])
      AC_CHECK_LIB([mystrings],[decimal_bin_size],,[
          AC_MSG_ERROR([Can't find decimal_bin_size])])])
    AC_MSG_CHECKING(for NdbApi headers)
     AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <NdbApi.hpp>]], [[int attr=NdbTransaction::Commit; ]])],[ndbapi_found="yes"],[])
    AS_IF([test "$ndbapi_found" = "yes"], 
       [AC_MSG_RESULT(found)],
       [AC_MSG_ERROR([Couldn't find NdbApi.hpp!])])
    AC_MSG_CHECKING(for NDB_LE_ThreadConfigLoop)
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <mgmapi.h>]], [[int attr=NDB_LE_ThreadConfigLoop; ]])],[have_cge63="yes"],[])
      AS_IF([test "$have_cge63" = "yes"],
        [AC_MSG_RESULT(found)
         HAVE_CGE63="-DCGE63"
         AC_SUBST(HAVE_CGE63)],
        [AC_MSG_RESULT(missing)])

    LDFLAGS="$LDFLAGS $LIBS"
  

    MYSQL_MAJOR_VERSION=`$MYSQL_CONFIG --version | sed -e 's/\.//g' -e 's/-//g' -e 's/[A-Za-z]//g' | cut -c1-2`

    case "$MYSQL_MAJOR_VERSION" in 
      50) AC_DEFINE(MYSQL_50, [1], [mysql5.0])
	;;
      51) AC_DEFINE(MYSQL_51, [1], [mysql5.1])
        ;;
      *) echo "Unsupported version of MySQL Detected!"
        ;;
     esac
    
    AC_SUBST(MYSQL_MAJOR_VERSION)
    AC_SUBST(MYSQL_CONFIG)
    
  
])

