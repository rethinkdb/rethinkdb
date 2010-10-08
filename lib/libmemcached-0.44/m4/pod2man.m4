AC_DEFUN([REQUIRE_POD2MAN],[
  AC_PATH_PROG([POD2MAN], [pod2man],
    "no", [$PATH:/usr/bin:/usr/local/bin:/usr/perl5/bin])
  AS_IF([test "x$POD2MAN" = "xno"],
    AC_MSG_ERROR(["Could not find pod2man anywhere in path"]))
  AC_SUBST(POD2MAN)
])
