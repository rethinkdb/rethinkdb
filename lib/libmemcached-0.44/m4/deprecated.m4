dnl ---------------------------------------------------------------------------
dnl Macro: deprecated
dnl ---------------------------------------------------------------------------
AC_DEFUN([ENABLE_DEPRECATED],[
  AC_ARG_ENABLE([deprecated],
    [AS_HELP_STRING([--enable-deprecated],
       [Enable deprecated interface @<:@default=off@:>@])],
    [ac_enable_deprecated="$enableval"],
    [ac_enable_deprecated="no"])

  AS_IF([test "$ac_enable_deprecated" = "yes"],
        [DEPRECATED="#define MEMCACHED_ENABLE_DEPRECATED 1"])
  AC_SUBST([DEPRECATED])
])
dnl ---------------------------------------------------------------------------
dnl End Macro: deprecated
dnl ---------------------------------------------------------------------------
