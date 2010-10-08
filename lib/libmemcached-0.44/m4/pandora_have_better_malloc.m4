dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([PANDORA_HAVE_BETTER_MALLOC],[
  AC_REQUIRE([AC_FUNC_MALLOC])
  AC_REQUIRE([AC_FUNC_REALLOC])
  AC_REQUIRE([AC_LIB_PREFIX])

  AC_ARG_ENABLE([umem],
    [AS_HELP_STRING([--enable-umem],
       [Enable linking with libumem @<:@default=off@:>@])],
    [ac_enable_umem="$enableval"],[
      case "$target_os" in
        *solaris*)
          ac_enable_umem="yes"
        ;;
        *)
          ac_enable_umem="no"
        ;;
      esac
    ])
  
  AC_ARG_ENABLE([tcmalloc],
    [AS_HELP_STRING([--enable-tcmalloc],
       [Enable linking with tcmalloc @<:@default=off@:>@])],
    [ac_enable_tcmalloc="$enableval"],
    [ac_enable_tcmalloc="no"])
  
  AC_ARG_ENABLE([mtmalloc],
    [AS_HELP_STRING([--disable-mtmalloc],
       [Enable linking with mtmalloc @<:@default=on@:>@])],
    [ac_enable_mtmalloc="$enableval"],
    [ac_enable_mtmalloc="yes"])
  
  save_LIBS="${LIBS}"
  LIBS=
  AS_IF([test "x$ac_enable_umem" = "xyes"],[
    AC_CHECK_LIB(umem,malloc,[],[])
  ],[
    case "$target_os" in
      *linux*)
        AS_IF([test "x$ac_enable_tcmalloc" != "xno"],[
          AC_CHECK_LIB(tcmalloc-minimal,malloc,[],[])
          AS_IF([test "x$ac_cv_lib_tcmalloc_minimal_malloc" != "xyes"],[
            AC_CHECK_LIB(tcmalloc,malloc,[],[])
          ])
        ])
        ;;
      *solaris*)
        AS_IF([test "x$ac_enable_mtmalloc" != "xno"],[
          AC_CHECK_LIB(mtmalloc,malloc,[],[])
        ])
        ;;
    esac
  ])
  BETTER_MALLOC_LIBS="${LIBS}"
  LIBS="${save_LIBS}"
  AC_SUBST([BETTER_MALLOC_LIBS])

])

AC_DEFUN([PANDORA_USE_BETTER_MALLOC],[
  AC_REQUIRE([PANDORA_HAVE_BETTER_MALLOC])
  LIBS="${LIBS} ${BETTER_MALLOC_LIBS}"
])

