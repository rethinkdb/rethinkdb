dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for libdl
#--------------------------------------------------------------------


AC_DEFUN([_PANDORA_SEARCH_LIBDL],[

  save_LIBS="$LIBS"
  LIBS=""
  AC_CHECK_LIB(dl,dlopen)
  AC_CHECK_FUNCS(dlopen)
  LIBDL_LIBS="$LIBS"
  LIBS="${save_LIBS}"
  AC_SUBST(LIBDL_LIBS)

  AM_CONDITIONAL(HAVE_LIBDL, [test "x${ac_cv_func_dlopen}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBDL],[

  AC_ARG_ENABLE([libdl],
    [AS_HELP_STRING([--disable-libdl],
      [Build with libdl support @<:@default=on@:>@])],
    [ac_enable_libdl="$enableval"],
    [ac_enable_libdl="yes"])

  _PANDORA_SEARCH_LIBDL
])


AC_DEFUN([PANDORA_HAVE_LIBDL],[
  AC_REQUIRE([_PANDORA_HAVE_LIBDL])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBDL],[
  ac_enable_libdl="yes"
  _PANDORA_SEARCH_LIBDL

  AS_IF([test "$ac_cv_func_dlopen" != "yes"],[
    AC_MSG_ERROR([libdl/dlopen() is required for ${PACKAGE}. On Debian this can be found in libc6-dev. On RedHat this can be found in glibc-devel.])
  ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBDL],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBDL])
])
