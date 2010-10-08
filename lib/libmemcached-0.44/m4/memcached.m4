AC_DEFUN([WITH_MEMCACHED],
  [AC_ARG_WITH([memcached],
    [AS_HELP_STRING([--with-memcached],
      [Memcached binary to use for make test])],
    [ac_cv_with_memcached="$withval"],
    [ac_cv_with_memcached=memcached])

  # just ignore the user if --without-memcached is passed.. it is
  # only used by make test
  AS_IF([test "x$withval" = "xno"],
    [
      ac_cv_with_memcached=memcached
      MEMC_BINARY=memcached
    ],
    [
       AS_IF([test -f "$withval"],
         [
           ac_cv_with_memcached=$withval
           MEMC_BINARY=$withval
         ],
         [
           AC_PATH_PROG([MEMC_BINARY], [$ac_cv_with_memcached], "no")
           AS_IF([test "x$MEMC_BINARY" = "xno"],
             AC_MSG_ERROR(["could not find memcached binary"]))
         ])
    ])

  AC_DEFINE_UNQUOTED([MEMCACHED_BINARY], "$MEMC_BINARY",
            [Name of the memcached binary used in make test])
  AC_SUBST(MEMC_BINARY)
])
