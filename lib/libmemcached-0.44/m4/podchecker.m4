AC_DEFUN([REQUIRE_PODCHECKER],[
  AC_PATH_PROG([PODCHECKER], [podchecker],
    "no", [$PATH:/usr/bin:/usr/local/bin:/usr/perl5/bin])
  AS_IF([test "x$PODCHECKER" = "xno"],
    AC_MSG_ERROR(["Could not find podchecker anywhere in path"]))
  AC_SUBST(PODCHECKER)
])
