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

AC_DEFUN([PANDORA_WITH_PYTHON3], [

  AC_REQUIRE([PANDORA_SWIG])

  AC_ARG_WITH([python3], 
    [AS_HELP_STRING([--with-python3],
      [Build Python3 Bindings @<:@default=yes@:>@])],[
        with_python3=$withval
        python3_requested=$withval
      ],[ 
        with_python3=yes
        python3_requested=no
      ])

  AS_IF([test "x$ac_cv_swig_has_python3_" != "xyes"],[
     with_python3=no
  ],[
   AS_IF([test "x$with_python3" != "xno"],[
     AS_IF([test "x$with_python3" != "xyes"],
       [PYTHON3=$with_python3],[
        AC_PATH_PROG([PYTHON3],[python3],[no])
        PANDORA_PYTHON3_DEVEL()
        AS_IF([test "x$python3exists" = "xno"],[with_python="no"])
     ]) 
   ])
  ])
  AS_IF([test "x$with_python3" = "xno" -a "$python3_requested" = "yes"],[
    AC_MSG_ERROR([Python3 support was explicity requested, but Python3 support
                  was not found. Please correct your build environment and try
                  again])
  ])
  AM_CONDITIONAL(BUILD_PYTHON3, [test "$with_python3" = "yes"])
])
