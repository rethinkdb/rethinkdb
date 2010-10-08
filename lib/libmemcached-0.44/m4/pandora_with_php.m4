dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl   
dnl pandora-build: A pedantic build system
dnl Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl
dnl From Monty Taylor


AC_DEFUN([PANDORA_WITH_PHP],[

  AC_ARG_WITH([php],
    [AS_HELP_STRING([--with-php],
      [Build NDB/PHP @<:@default=no@:>@])],
    [with_php=$withval],
    [with_php=no])

  AS_IF([test "x$with_php" != "xno"],[
    dnl We explicitly requested PHP build. Fail on too-young SWIG.
    AS_IF([test "x$SWIG_CAN_BUILD_PHP" != "xyes"],
      [AC_MSG_ERROR("Your version of SWIG is too young to build NDB/PHP. >=1.3.33 is required!")])
    AS_IF([test "x$with_php" != "xyes"],
      [ac_check_php_config=$with_php],
      [ac_check_php_config="php-config php-config5"])
      AC_CHECK_PROGS(PHP_CONFIG, [$ac_check_php_config])
    ])

  AS_IF([test "x$PHP_CONFIG" != "x"],[
    PHP_CFLAGS=`$PHP_CONFIG --includes`
    PHP_CPPFLAGS=`$PHP_CONFIG --includes`
    PHP_LDFLAGS=`$PHP_CONFIG --ldflags`
    PHP_EXTDIR=`$PHP_CONFIG --extension-dir`
    strip_php_prefix=`$PHP_CONFIG --prefix | sed 's/\//./g'`
    PHP_ARCH_DIR=`echo $PHP_EXTDIR | sed "s/$strip_php_prefix//"`
  ],[
    PHP_CFLAGS=
    PHP_CPPFLAGS=
    PHP_LDFLAGS=
    PHP_EXTDIR=
    PHP_ARCH_DIR=
    with_php=no
  ])

  AC_SUBST(PHP_CFLAGS)
  AC_SUBST(PHP_CPPFLAGS)
  AC_SUBST(PHP_LDFLAGS)
  AC_SUBST(PHP_EXTDIR)
  AC_SUBST(PHP_ARCH_DIR)

  AM_CONDITIONAL(BUILD_PHP, test "$with_php" = "yes")
  
])

