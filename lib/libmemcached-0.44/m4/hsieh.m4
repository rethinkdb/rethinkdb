dnl ---------------------------------------------------------------------------
dnl Macro: ENABLE_HSIEH_HASH
dnl ---------------------------------------------------------------------------
AC_DEFUN([ENABLE_HSIEH_HASH],
  [AC_ARG_ENABLE([hsieh_hash],
    [AS_HELP_STRING([--enable-hsieh_hash],
      [build with support for hsieh hashing. @<:default=off@:>@])],
    [ac_cv_enable_hsieh_hash=yes],
    [ac_cv_enable_hsieh_hash=no])

  AS_IF([test "$ac_cv_enable_hsieh_hash" = "yes"],
        [AC_DEFINE([HAVE_HSIEH_HASH], [1], [Enables hsieh hashing support])])

  AM_CONDITIONAL([INCLUDE_HSIEH_SRC], [test "$ac_cv_enable_hsieh_hash" = "yes"])
])
dnl ---------------------------------------------------------------------------
dnl End Macro: ENABLE_HSIEH_HASH
dnl ---------------------------------------------------------------------------
