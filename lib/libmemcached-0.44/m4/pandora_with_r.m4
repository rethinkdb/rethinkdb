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

AC_DEFUN([PANDORA_WITH_R],[
    dnl Check for GNU R
    AC_ARG_WITH([r], 
    [AS_HELP_STRING([--with-r],
      [Build R Bindings @<:@default=yes@:>@])],
    [with_r=$withval], 
    [with_r=yes])

  AS_IF([test "x$with_r" != "xno"],[

    PKG_CHECK_MODULES([R], [libR], [
      with_r=yes
    ],[
      with_r=no
    ])

   AC_SUBST(R_CFLAGS)
   AC_SUBST(R_LIBS)
  ])
  AM_CONDITIONAL(BUILD_R, test "$with_r" = "yes")

])
