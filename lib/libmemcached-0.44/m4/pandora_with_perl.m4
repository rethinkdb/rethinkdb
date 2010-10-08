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


AC_DEFUN([PANDORA_WITH_PERL], [

  AC_ARG_WITH([perl], 
    [AS_HELP_STRING([--with-perl],
      [Build Perl Bindings @<:@default=yes@:>@])],
    [with_perl=$withval], 
    [with_perl=yes])

  AC_ARG_WITH([perl-arch],
    [AS_HELP_STRING([--with-perl-arch],
      [Install Perl bindings into system location @<:@default=no@:>@])],
    [with_perl_arch=$withval],
    [with_perl_arch=no])

  AS_IF([test "x$with_perl" != "xno"],[
    AS_IF([test "x$with_perl" != "xyes"],
      [ac_chk_perl=$with_perl],
      [ac_chk_perl=perl])
    AC_CHECK_PROGS(PERL,$ac_chk_perl)
  ])
  AS_IF([test "x$PERL" != "x"],[
    AC_CACHE_CHECK([for Perl include path],[pandora_cv_perl_include],[
      pandora_cv_perl_include=`$PERL -MConfig -e 'print $Config{archlib};'`
      pandora_cv_perl_include="${pandora_cv_perl_include}/CORE"
    ])
    AC_CACHE_CHECK([for Perl CPPFLAGS],[pandora_cv_perl_cppflags],[
      pandora_cv_perl_cppflags=`$PERL -MConfig -e 'print $Config{cppflags};'`
      pandora_cv_perl_cppflags="${pandora_cv_perl_cppflags}"
    ])
    PERL_CPPFLAGS="-I${pandora_cv_perl_include} ${pandora_cv_perl_cppflags}"

    AC_CACHE_CHECK([for Perl development headers],
      [pandora_cv_perl_dev],[
        
        save_CPPFLAGS="${CPPFLAGS}"
        CPPFLAGS="${CPPFLAGS} ${PERL_CPPFLAGS}"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <math.h>
#include <stdlib.h>
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
        ]])],
        [pandora_cv_perl_dev=yes],
        [pandora_cv_perl_dev=no])
        CPPFLAGS="${save_CPPFLAGS}"
      ])

    AS_IF([test "${pandora_cv_perl_dev}" = "no"],
      [with_perl=no])

    AC_CACHE_CHECK([for Perl install location],
      [pandora_cv_perl_archdir],[
        AS_IF([test "${with_perl_arch}" = "no"],[
          pandora_cv_perl_archdir=`$PERL -MConfig -e 'print $Config{sitearch}'`
        ],[
          pandora_cv_perl_archdir=`$PERL -MConfig -e 'print $Config{archlib}'`
        ])
        pandora_cv_perl_archdir="${pandora_cv_perl_archdir}"
    ])
 
    PERL_ARCHDIR="${pandora_cv_perl_archdir}"
  ])
  AC_SUBST([PERL_CPPFLAGS])
  AC_SUBST([PERL_ARCHDIR])

  AM_CONDITIONAL(BUILD_PERL, [test "$with_perl" != "no"])

])
