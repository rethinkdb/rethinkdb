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

AC_DEFUN([PANDORA_PLATFORM],[

  dnl Canonicalize the configuration name.

  AC_DEFINE_UNQUOTED([HOST_VENDOR], ["$host_vendor"],[Vendor of Build System])
  AC_DEFINE_UNQUOTED([HOST_OS], ["$host_os"], [OS of Build System])
  AC_DEFINE_UNQUOTED([HOST_CPU], ["$host_cpu"], [CPU of Build System])

  AC_DEFINE_UNQUOTED([TARGET_VENDOR], ["$target_vendor"],[Vendor of Target System])
  AC_DEFINE_UNQUOTED([TARGET_OS], ["$target_os"], [OS of Target System])
  AC_DEFINE_UNQUOTED([TARGET_CPU], ["$target_cpu"], [CPU of Target System])


  case "$host_os" in
    *solaris*|*freebsd*)
    AS_IF([test "x${ac_cv_env_CPPFLAGS_set}" = "x"],[
      CPPFLAGS="${CPPFLAGS} -I/usr/local/include"
    ])

    AS_IF([test "x${ac_cv_env_LDFLAGS_set}" = "x"],[
      LDFLAGS="${LDFLAGS} -L/usr/local/lib"
    ])
    ;;
  esac

  PANDORA_OPTIMIZE_BITFIELD=1

  case "$target_os" in
    *linux*)
    TARGET_LINUX="true"
    AC_SUBST(TARGET_LINUX)
    AC_DEFINE([TARGET_OS_LINUX], [1], [Whether we build for Linux])
      ;;
    *darwin*)
      TARGET_OSX="true"
      AC_SUBST(TARGET_OSX)
      AC_DEFINE([TARGET_OS_OSX], [1], [Whether we build for OSX])
      ;;
    *solaris*)
      TARGET_SOLARIS="true"
      PANDORA_OPTIMIZE_BITFIELD=0
      AC_SUBST(TARGET_SOLARIS)
      AC_DEFINE([TARGET_OS_SOLARIS], [1], [Whether we are building for Solaris])
      ;;
    *freebsd*)
      TARGET_FREEBSD="true"
      AC_SUBST(TARGET_FREEBSD)
      AC_DEFINE([TARGET_OS_FREEBSD], [1], [Whether we are building for FreeBSD])
      AC_DEFINE([__APPLE_CC__],[1],[Workaround for bug in FreeBSD headers])
      ;;
    *)
      ;;
  esac

  AC_SUBST(PANDORA_OPTIMIZE_BITFIELD)

  AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
  AC_CHECK_DECL([__ICC], [INTELCC="yes"], [INTELCC="no"])

  AS_IF([test "$INTELCC" = "yes"], [enable_rpath=no])
 
  dnl By default, Sun Studio grabs special versions of limits.h and string.h
  dnl when you use <cstring> and <climits>. By setting this define, we can
  dnl disable that and cause those to wrap the standard headers instead.
  dnl http://www.stlport.com/doc/configure.html
  AS_IF([test "$SUNCC" = "yes"],[
    AC_DEFINE([_STLP_NO_NEW_C_HEADERS],[1],
      [Cause Sun Studio to not be quite so strict with standards conflicts])
  ])

  AS_IF([test "x$TARGET_OSX" = "xtrue"],[
    AS_IF([test "x$ac_enable_fat_binaries" = "xyes"],[
      AM_CFLAGS="-arch i386 -arch x86_64 -arch ppc"
      AM_CXXFLAGS="-arch i386 -arch x86_64 -arch ppc"
      AM_LDFLAGS="-arch i386 -arch x86_64 -arch ppc"
    ])
  ])

])
