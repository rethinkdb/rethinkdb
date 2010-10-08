dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl ---------------------------------------------------------------------------
dnl Macro: PANDORA_64BIT
dnl ---------------------------------------------------------------------------
AC_DEFUN([PANDORA_64BIT],[
  AC_BEFORE([$0], [AC_LIB_PREFIX])

  AC_ARG_ENABLE([64bit],
    [AS_HELP_STRING([--disable-64bit],
      [Build 64 bit binary @<:@default=on@:>@])],
    [ac_enable_64bit="$enableval"],
    [ac_enable_64bit="yes"])

  AC_CHECK_PROGS(ISAINFO, [isainfo], [no])
  AS_IF([test "x$ISAINFO" != "xno"],
        [isainfo_b=`${ISAINFO} -b`],
        [isainfo_b="x"])

  AS_IF([test "$isainfo_b" != "x"],[

    isainfo_k=`${ISAINFO} -k` 
    DTRACEFLAGS="${DTRACEFLAGS} -${isainfo_b}"

    AS_IF([test "x$ac_enable_64bit" = "xyes"],[

      AS_IF([test "x${ac_cv_env_LDFLAGS_set}" = "x"],[
        LDFLAGS="-L/usr/local/lib/${isainfo_k} ${LDFLAGS}"
      ])

      AS_IF([test "x$libdir" = "x\${exec_prefix}/lib"],[
       dnl The user hasn't overridden the default libdir, so we'll 
       dnl the dir suffix to match solaris 32/64-bit policy
       libdir="${libdir}/${isainfo_k}"
      ])

      AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"],[
        CFLAGS="${CFLAGS} -m64"
        ac_cv_env_CFLAGS_set=set
        ac_cv_env_CFLAGS_value='-m64'
      ])
      AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],[
        CXXFLAGS="${CXXFLAGS} -m64"
        ac_cv_env_CXXFLAGS_set=set
        ac_cv_env_CXXFLAGS_value='-m64'
      ])

      AS_IF([test "$target_cpu" = "sparc" -a "x$SUNCC" = "xyes"],[
        AM_CFLAGS="-xmemalign=8s ${AM_CFLAGS}"
        AM_CXXFLAGS="-xmemalign=8s ${AM_CXXFLAGS}"
      ])
    ])
  ])
])
dnl ---------------------------------------------------------------------------
dnl End Macro: PANDORA_64BIT
dnl ---------------------------------------------------------------------------
