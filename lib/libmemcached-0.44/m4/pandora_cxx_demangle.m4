dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Check for all of the headers and libs that Drizzle needs. We check all
dnl of these for plugins too, to ensure that all of the appropriate defines
dnl are set.

AC_DEFUN([PANDORA_CXX_DEMANGLE],[
  AC_LANG_PUSH([C++])
  save_CXXFLAGS="${CXXFLAGS}"
  CXXFLAGS="${CXX_STANDARD} ${CXXFLAGS}"
  AC_CHECK_HEADERS(cxxabi.h)
  AC_CACHE_CHECK([checking for abi::__cxa_demangle], pandora_cv_cxa_demangle,
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <cxxabi.h>]], [[
    char *foo= 0; int bar= 0;
    foo= abi::__cxa_demangle(foo, foo, 0, &bar);
  ]])],[pandora_cv_cxa_demangle=yes],[pandora_cv_cxa_demangle=no])])
  CXXFLAGS="${save_CXXFLAGS}"
  AC_LANG_POP()

  AS_IF([test "x$pandora_cv_cxa_demangle" = xyes],[
    AC_DEFINE(HAVE_ABI_CXA_DEMANGLE, 1,
              [Define to 1 if you have the `abi::__cxa_demangle' function.])
  ])
])
