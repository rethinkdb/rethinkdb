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

AC_DEFUN([PANDORA_SWIG],[

  AC_PROG_SWIG(1.3.31)
  
  AC_DEFINE_UNQUOTED([SWIG_TYPE_TABLE],
    [$PACKAGE],
    [Type Table name for SWIG symbol table])

  dnl Have to hard-code /usr/local/include and /usr/include into the path.
  dnl I hate this. Why is swig sucking me
  SWIG="$SWIG \${DEFS} -I\${top_srcdir} -I\${top_builddir} -I/usr/local/include -I/usr/include"
  AC_SUBST([SWIG])


])

AC_DEFUN([PANDORA_SWIG_PYTHON3],[
  AC_REQUIRE([PANDORA_SWIG])
  AS_IF([test "x$SWIG" != "x"],[
    AC_CACHE_CHECK([if swig supports Python3],
      [ac_cv_swig_has_python3_],
      [
        AS_IF([$SWIG -python -help 2>&1 | grep py3 > /dev/null],
          [ac_cv_swig_has_python3_=yes],
          [ac_cv_swig_has_python3_=no])
      ])
  ])
])
