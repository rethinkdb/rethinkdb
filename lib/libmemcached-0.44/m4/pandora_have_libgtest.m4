dnl Copyright (C) 2010 Monty Taylor
dnl This file is free software; Monty Taylor
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_LIBGTEST],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for libgtest
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([libgtest],
    [AS_HELP_STRING([--disable-libgtest],
      [Build with libgtest support @<:@default=on@:>@])],
    [ac_enable_libgtest="$enableval"],
    [ac_enable_libgtest="yes"])

  AS_IF([test "x$ac_enable_libgtest" = "xyes"],[
    AC_LANG_PUSH(C++)
    AC_LIB_HAVE_LINKFLAGS(gtest,,[
      #include <gtest/gtest.h>
TEST(pandora_test_libgtest, PandoraTest)
{
  ASSERT_EQ(1, 1);
}
    ],[])
    AC_LANG_POP()
  ],[
    ac_cv_libgtest="no"
  ])

  AM_CONDITIONAL(HAVE_LIBGTEST, [test "x${ac_cv_libgtest}" = "xyes"])
])

AC_DEFUN([PANDORA_HAVE_LIBGTEST],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBGTEST])
])

AC_DEFUN([PANDORA_REQUIRE_LIBGTEST],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBGTEST])
  AS_IF([test "x${ac_cv_libgtest}" = "xno"],
    AC_MSG_ERROR([libgtest is required for ${PACKAGE}]))
])
