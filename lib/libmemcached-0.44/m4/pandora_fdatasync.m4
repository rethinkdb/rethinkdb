dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for a working fdatasync call
#--------------------------------------------------------------------


AC_DEFUN([PANDORA_WORKING_FDATASYNC],[
  AC_CACHE_CHECK([working fdatasync],[ac_cv_func_fdatasync],[
    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
      ]],[[
fdatasync(4);
      ]])],
    [ac_cv_func_fdatasync=yes],
    [ac_cv_func_fdatasync=no])
    AC_LANG_POP()
  ])
  AS_IF([test "x${ac_cv_func_fdatasync}" = "xyes"],
    [AC_DEFINE([HAVE_FDATASYNC],[1],[If the system has a working fdatasync])])
])
