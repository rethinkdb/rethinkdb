dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl   
dnl pandora-build: A pedantic build system
dnl Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystem
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl
dnl From Monty Taylor
dnl
dnl Test if we can Use -pipe to avoid making temp files during the compile.
dnl Should speed up compile on slower disks

AC_DEFUN([PANDORA_USE_PIPE],[

  AS_IF([test "$GCC" = "yes"],[
    AC_CACHE_CHECK([for working -pipe], [pandora_cv_use_pipe], [
      AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>

int main(int argc, char** argv)
{
  (void) argc; (void) argv;
  return 0;
}
      ]])],
      [pandora_cv_use_pipe=yes],
      [pandora_cv_use_pipe=no])
    ])
    AS_IF([test "$pandora_cv_use_pipe" = "yes"],[
      AM_CFLAGS="-pipe ${AM_CFLAGS}"
      AM_CXXFLAGS="-pipe ${AM_CXXFLAGS}"
    ])
  ])
])
