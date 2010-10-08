AC_DEFUN([ENABLE_UTILLIB],[
  AC_ARG_ENABLE([utils],
    [AS_HELP_STRING([--disable-utils],
       [Disable libmemcachedutils @<:@default=on@:>@])],
    [BUILD_UTILLIB="$enableval"],
    [BUILD_UTILLIB="yes"])

  if test "x$BUILD_UTILLIB" = "xyes"; then
    if test x"$acx_pthread_ok" != "xyes"; then
      AC_MSG_ERROR([Sorry you need POSIX thread library to build libmemcachedutil.])
    fi
    AC_DEFINE([HAVE_LIBMEMCACHEDUTIL], [1], [Enables libmemcachedutil Support])
  fi

  AM_CONDITIONAL([BUILD_LIBMEMCACHEDUTIL],[test "x$BUILD_UTILLIB" = "xyes"])
])
