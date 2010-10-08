dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Which version of the canonical setup we're using
AC_DEFUN([PANDORA_CANONICAL_VERSION],[0.134])

AC_DEFUN([PANDORA_FORCE_DEPEND_TRACKING],[
  AC_ARG_ENABLE([fat-binaries],
    [AS_HELP_STRING([--enable-fat-binaries],
      [Enable fat binary support on OSX @<:@default=off@:>@])],
    [ac_enable_fat_binaries="$enableval"],
    [ac_enable_fat_binaries="no"])

  dnl Force dependency tracking on for Sun Studio builds
  AS_IF([test "x${enable_dependency_tracking}" = "x"],[
    enable_dependency_tracking=yes
  ])
  dnl If we're building OSX Fat Binaries, we have to turn off -M options
  AS_IF([test "x${ac_enable_fat_binaries}" = "xyes"],[
    enable_dependency_tracking=no
  ])
])

dnl The standard setup for how we build Pandora projects
AC_DEFUN([PANDORA_CANONICAL_TARGET],[
  AC_REQUIRE([PANDORA_FORCE_DEPEND_TRACKING])
  ifdef([m4_define],,[define([m4_define],   defn([define]))])
  ifdef([m4_undefine],,[define([m4_undefine],   defn([undefine]))])
  m4_define([PCT_ALL_ARGS],[$*])
  m4_define([PCT_REQUIRE_CXX],[no])
  m4_define([PCT_FORCE_GCC42],[no])
  m4_define([PCT_DONT_SUPPRESS_INCLUDE],[no])
  m4_define([PCT_VERSION_FROM_VC],[no])
  m4_define([PCT_USE_VISIBILITY],[yes])
  m4_foreach([pct_arg],[$*],[
    m4_case(pct_arg,
      [require-cxx], [
        m4_undefine([PCT_REQUIRE_CXX])
        m4_define([PCT_REQUIRE_CXX],[yes])
      ],
      [force-gcc42], [
        m4_undefine([PCT_FORCE_GCC42])
        m4_define([PCT_FORCE_GCC42],[yes])
      ],
      [skip-visibility], [
        m4_undefine([PCT_USE_VISIBILITY])
        m4_define([PCT_USE_VISIBILITY],[no])
      ],
      [dont-suppress-include], [
        m4_undefine([PCT_DONT_SUPPRESS_INCLUDE])
        m4_define([PCT_DONT_SUPPRESS_INCLUDE],[yes])
      ],
      [version-from-vc], [
        m4_undefine([PCT_VERSION_FROM_VC])
        m4_define([PCT_VERSION_FROM_VC],[yes])
    ])
  ])

  AC_CONFIG_MACRO_DIR([m4])

  m4_if(m4_substr(m4_esyscmd(test -d src && echo 0),0,1),0,[
    AC_CONFIG_HEADERS([src/config.h])
  ],[
    AC_CONFIG_HEADERS([config.h])
  ])

  # We need to prevent canonical target
  # from injecting -O2 into CFLAGS - but we won't modify anything if we have
  # set CFLAGS on the command line, since that should take ultimate precedence
  AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"],
        [CFLAGS=""])
  AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"],
        [CXXFLAGS=""])
  
  AC_CANONICAL_TARGET
  
  m4_if(PCT_DONT_SUPRESS_INCLUDE,yes,[
    AM_INIT_AUTOMAKE(-Wall -Werror -Wno-portability subdir-objects foreign)
  ],[
    AM_INIT_AUTOMAKE(-Wall -Werror -Wno-portability nostdinc subdir-objects foreign)
  ])

  m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

  m4_if(m4_substr(m4_esyscmd(test -d gnulib && echo 0),0,1),0,[
    gl_EARLY
  ])
  
  AC_REQUIRE([AC_PROG_CC])
  m4_if(PCT_FORCE_GCC42, [yes], [
    AC_REQUIRE([PANDORA_ENSURE_GCC_VERSION])
  ])
  AC_REQUIRE([PANDORA_64BIT])

  m4_if(PCT_VERSION_FROM_VC,yes,[
    PANDORA_VC_VERSION
  ],[
    PANDORA_TEST_VC_DIR
  ])
  PANDORA_VERSION

  dnl Once we can use a modern autoconf, we can use this
  dnl AC_PROG_CC_C99
  AC_REQUIRE([AC_PROG_CXX])
  PANDORA_EXTENSIONS
  AM_PROG_CC_C_O



  PANDORA_PLATFORM

  PANDORA_LIBTOOL

  dnl autoconf doesn't automatically provide a fail-if-no-C++ macro
  dnl so we check c++98 features and fail if we don't have them, mainly
  dnl for that reason
  PANDORA_CHECK_CXX_STANDARD
  m4_if(PCT_REQUIRE_CXX, [yes], [
    AS_IF([test "$ac_cv_cxx_stdcxx_98" = "no"],[
      AC_MSG_ERROR([No working C++ Compiler has been found. ${PACKAGE} requires a C++ compiler that can handle C++98])
    ])

  ])
  
  m4_if(m4_substr(m4_esyscmd(test -d gnulib && echo 0),0,1),0,[
    gl_INIT
    AC_CONFIG_LIBOBJ_DIR([gnulib])
  ])

  PANDORA_CHECK_C_VERSION
  PANDORA_CHECK_CXX_VERSION

  AC_C_BIGENDIAN
  AC_C_CONST
  AC_C_INLINE
  AC_C_VOLATILE
  AC_C_RESTRICT

  AC_HEADER_TIME
  AC_STRUCT_TM
  AC_TYPE_SIZE_T
  AC_SYS_LARGEFILE
  PANDORA_CLOCK_GETTIME

  # off_t is not a builtin type
  AC_CHECK_SIZEOF(off_t, 4)
  AS_IF([test "$ac_cv_sizeof_off_t" -eq 0],[
    AC_MSG_ERROR("${PACKAGE} needs an off_t type.")
  ])

  AC_CHECK_SIZEOF(size_t)
  AS_IF([test "$ac_cv_sizeof_size_t" -eq 0],[
    AC_MSG_ERROR("${PACKAGE} needs an size_t type.")
  ])

  AC_DEFINE_UNQUOTED([SIZEOF_SIZE_T],[$ac_cv_sizeof_size_t],[Size of size_t as computed by sizeof()])
  AC_CHECK_SIZEOF(long long)
  AC_DEFINE_UNQUOTED([SIZEOF_LONG_LONG],[$ac_cv_sizeof_long_long],[Size of long long as computed by sizeof()])
  AC_CACHE_CHECK([if time_t is unsigned], [ac_cv_time_t_unsigned],[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[
#include <time.h>
      ]],
      [[
      int array[(((time_t)-1) > 0) ? 1 : -1];
      ]])
    ],[
      ac_cv_time_t_unsigned=yes
    ],[
      ac_cv_time_t_unsigned=no
    ])
  ])
  AS_IF([test "$ac_cv_time_t_unsigned" = "yes"],[
    AC_DEFINE([TIME_T_UNSIGNED], 1, [Define to 1 if time_t is unsigned])
  ])

  AC_CHECK_LIBM
  dnl Bug on FreeBSD - LIBM check doesn't set the damn variable
  AC_SUBST([LIBM])
  
  AC_CHECK_FUNC(setsockopt, [], [AC_CHECK_LIB(socket, setsockopt)])
  AC_CHECK_FUNC(bind, [], [AC_CHECK_LIB(bind, bind)])



  PANDORA_OPTIMIZE

  AC_LANG_PUSH(C++)
  # Test whether madvise() is declared in C++ code -- it is not on some
  # systems, such as Solaris
  AC_CHECK_DECLS([madvise], [], [], [AC_INCLUDES_DEFAULT[
  #if HAVE_SYS_MMAN_H
  #include <sys/types.h>
  #include <sys/mman.h>
  #endif
  ]])
  AC_LANG_POP()

  PANDORA_HAVE_GCC_ATOMICS

  m4_if(PCT_USE_VISIBILITY,[yes],[
    dnl We need to inject error into the cflags to test if visibility works or not
    save_CFLAGS="${CFLAGS}"
    CFLAGS="${CFLAGS} -Werror"
    PANDORA_VISIBILITY
    CFLAGS="${save_CFLAGS}"
  ])

  PANDORA_HEADER_ASSERT

  PANDORA_WARNINGS(PCT_ALL_ARGS)

  PANDORA_ENABLE_DTRACE

  AC_LIB_PREFIX
  PANDORA_HAVE_BETTER_MALLOC

  AC_CHECK_PROGS([DOXYGEN], [doxygen])
  AC_CHECK_PROGS([PERL], [perl])
  AC_CHECK_PROGS([DPKG_GENSYMBOLS], [dpkg-gensymbols], [:])

  AM_CONDITIONAL(HAVE_DPKG_GENSYMBOLS,[test "x${DPKG_GENSYMBOLS}" != "x:"])

  PANDORA_WITH_GETTEXT

  AS_IF([test "x${gl_LIBOBJS}" != "x"],[
    AS_IF([test "$GCC" = "yes"],[
      AM_CPPFLAGS="-isystem \${top_srcdir}/gnulib -isystem \${top_builddir}/gnulib ${AM_CPPFLAGS}"
    ],[
    AM_CPPFLAGS="-I\${top_srcdir}/gnulib -I\${top_builddir}/gnulib ${AM_CPPFLAGS}"
    ])
  ])
  m4_if(m4_substr(m4_esyscmd(test -d src && echo 0),0,1),0,[
    AM_CPPFLAGS="-I\$(top_srcdir)/src -I\$(top_builddir)/src ${AM_CPPFLAGS}"
  ],[
    AM_CPPFLAGS="-I\$(top_srcdir) -I\$(top_builddir) ${AM_CPPFLAGS}"
  ])

  PANDORA_USE_PIPE


  AM_CFLAGS="${AM_CFLAGS} ${CC_WARNINGS} ${CC_PROFILING} ${CC_COVERAGE}"
  AM_CXXFLAGS="${AM_CXXFLAGS} ${CXX_WARNINGS} ${CC_PROFILING} ${CC_COVERAGE}"

  AC_SUBST([AM_CFLAGS])
  AC_SUBST([AM_CXXFLAGS])
  AC_SUBST([AM_CPPFLAGS])
  AC_SUBST([AM_LDFLAGS])

])
