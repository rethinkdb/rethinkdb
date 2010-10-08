dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Check for all of the headers and libs that Drizzle needs. We check all
dnl of these for plugins too, to ensure that all of the appropriate defines
dnl are set.

AC_DEFUN([PANDORA_DRIZZLE_BUILD],[

  PANDORA_HAVE_CXX_UNORDERED

  PANDORA_CXX_CSTDINT
  PANDORA_CXX_CINTTYPES

  AC_STRUCT_TM

  AC_FUNC_ALLOCA
  AC_FUNC_UTIME_NULL
  AC_FUNC_VPRINTF

  PANDORA_WORKING_FDATASYNC

  AC_CHECK_FUNCS(\
    gethrtime \
    setupterm \
    backtrace \
    backtrace_symbols \
    backtrace_symbols_fd)

  AC_HEADER_STAT
  AC_HEADER_DIRENT
  AC_HEADER_STDC
  AC_HEADER_SYS_WAIT
  AC_HEADER_STDBOOL

  AC_CHECK_HEADERS(sys/types.h sys/fpu.h fpu_control.h ieeefp.h)
  AC_CHECK_HEADERS(select.h sys/select.h)
  AC_CHECK_HEADERS(utime.h sys/utime.h )
  AC_CHECK_HEADERS(synch.h sys/mman.h sys/socket.h)
  AC_CHECK_HEADERS(sched.h)
  AC_CHECK_HEADERS(sys/prctl.h)
  AC_CHECK_HEADERS(execinfo.h)
  AC_CHECK_HEADERS(locale.h)
  AC_CHECK_HEADERS(termcap.h termio.h termios.h asm/termbits.h)
  AC_CHECK_HEADERS(paths.h)

  
  #--------------------------------------------------------------------
  # Check for system libraries. Adds the library to $LIBS
  # and defines HAVE_LIBM etc
  #--------------------------------------------------------------------
  
    # For the sched_yield() function on Solaris
  AC_CHECK_FUNC(sched_yield, [],
    [AC_CHECK_LIB(posix4, [sched_yield],
      [AC_DEFINE(HAVE_SCHED_YIELD, 1, [Have sched_yield function]) LIBS="$LIBS -lposix4"])])
  
  AS_IF([test "$ac_cv_header_termio_h" = "no" -a "$ac_cv_header_termios_h" = "no"],[
    AC_CHECK_FUNC(gtty, [], [AC_CHECK_LIB(compat, gtty)])
  ])
  
  AC_CHECK_HEADERS([curses.h term.h],[],[],[[
    #ifdef HAVE_CURSES_H
    # include <curses.h>
    #endif
  ]])
  AC_CHECK_TYPES([uint, ulong])

  PANDORA_CXX_DEMANGLE
  PANDORA_REQUIRE_BOOST([1.38])
  PANDORA_REQUIRE_BOOST_PROGRAM_OPTIONS


  AH_TOP([
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "config/top.h"
])
  mkdir -p config
  cat > config/top.h.stamp <<EOF_CONFIG_TOP

#if defined(i386) && !defined(__i386__)
#define __i386__
#endif

#if defined(_FILE_OFFSET_BITS)
# undef _FILE_OFFSET_BITS
#endif
EOF_CONFIG_TOP
  diff config/top.h.stamp config/top.h >/dev/null 2>&1 || mv config/top.h.stamp config/top.h
  rm -f config/top.h.stamp


  AH_BOTTOM([
#if defined(__cplusplus)
# include CSTDINT_H
# include CINTTYPES_H
#else
# include <stdint.h>
# include <inttypes.h>
#endif

#if !defined(HAVE_ULONG) && !defined(__USE_MISC)
typedef unsigned long int ulong;
#endif

#endif /* __CONFIG_H__ */
  ])
])
