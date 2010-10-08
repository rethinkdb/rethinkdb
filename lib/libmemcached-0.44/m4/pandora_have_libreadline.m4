#
# SYNOPSIS
#
#   PANDORA_HAVE_LIBREADLINE
#
# DESCRIPTION
#
#   Searches for a readline compatible library. If found, defines
#   `HAVE_LIBREADLINE'. If the found library has the `add_history'
#   function, sets also `HAVE_READLINE_HISTORY'. Also checks for the
#   locations of the necessary include files and sets `HAVE_READLINE_H'
#   or `HAVE_READLINE_READLINE_H' and `HAVE_READLINE_HISTORY_H' or
#   'HAVE_HISTORY_H' if the corresponding include files exists.
#
#   The libraries that may be readline compatible are `libedit',
#   `libeditline' and `libreadline'. Sometimes we need to link a
#   termcap library for readline to work, this macro tests these cases
#   too by trying to link with `libtermcap', `libcurses' or
#   `libncurses' before giving up.
#
#   Here is an example of how to use the information provided by this
#   macro to perform the necessary includes or declarations in a C
#   file:
#
#     #ifdef HAVE_LIBREADLINE
#     #  if defined(HAVE_READLINE_READLINE_H)
#     #    include <readline/readline.h>
#     #  elif defined(HAVE_READLINE_H)
#     #    include <readline.h>
#     #  else /* !defined(HAVE_READLINE_H) */
#     extern char *readline ();
#     #  endif /* !defined(HAVE_READLINE_H) */
#     char *cmdline = NULL;
#     #else /* !defined(HAVE_READLINE_READLINE_H) */
#       /* no readline */
#     #endif /* HAVE_LIBREADLINE */
#
#     #ifdef HAVE_READLINE_HISTORY
#     #  if defined(HAVE_READLINE_HISTORY_H)
#     #    include <readline/history.h>
#     #  elif defined(HAVE_HISTORY_H)
#     #    include <history.h>
#     #  else /* !defined(HAVE_HISTORY_H) */
#     extern void add_history ();
#     extern int write_history ();
#     extern int read_history ();
#     #  endif /* defined(HAVE_READLINE_HISTORY_H) */
#       /* no history */
#     #endif /* HAVE_READLINE_HISTORY */
#
# LAST MODIFICATION
#
#   2009-11-17
#
# Based on VL_LIB_READLINE from  Ville Laurikari
#
# COPYLEFT
#
#   Copyright (c) 2009 Monty Taylor
#   Copyright (c) 2002 Ville Laurikari <vl@iki.fi>
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([PANDORA_CHECK_TIOCGWINSZ],[
  AC_CACHE_CHECK([for TIOCGWINSZ in sys/ioctl.h],
    [pandora_cv_tiocgwinsz_in_ioctl],[
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/ioctl.h>
      ]],[[
int x= TIOCGWINSZ;
      ]])
    ],[
      pandora_cv_tiocgwinsz_in_ioctl=yes
    ],[
      pandora_cv_tiocgwinsz_in_ioctl=no
    ])
  ])
  AS_IF([test "$pandora_cv_tiocgwinsz_in_ioctl" = "yes"],[   
    AC_DEFINE([GWINSZ_IN_SYS_IOCTL], [1],
              [READLINE: your system defines TIOCGWINSZ in sys/ioctl.h.])
  ])
])

AC_DEFUN([PANDORA_CHECK_RL_COMPENTRY], [
  AC_CACHE_CHECK([defined rl_compentry_func_t], [pandora_cv_rl_compentry],[
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[
#include "stdio.h"
#include "readline/readline.h"
      ]],[[
rl_compentry_func_t *func2= (rl_compentry_func_t*)0;
      ]])
    ],[
      pandora_cv_rl_compentry=yes
    ],[
      pandora_cv_rl_compentry=no
    ])
  ])
  AS_IF([test "$pandora_cv_rl_compentry" = "yes"],[
    AC_DEFINE([HAVE_RL_COMPENTRY], [1],
              [Does system provide rl_compentry_func_t])
  ])

  save_CXXFLAGS="${CXXFLAGS}"
  CXXFLAGS="${AM_CXXFLAGS} ${CXXFLAGS}"
  AC_LANG_PUSH(C++)
  AC_CACHE_CHECK([rl_compentry_func_t works], [pandora_cv_rl_compentry_works],[
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[
#include "stdio.h"
#include "readline/readline.h"
      ]],[[
rl_completion_entry_function= (rl_compentry_func_t*)NULL;
      ]])
    ],[
      pandora_cv_rl_compentry_works=yes
    ],[
      pandora_cv_rl_compentry_works=no
    ])
  ])
  AS_IF([test "$pandora_cv_rl_compentry_works" = "yes"],[
    AC_DEFINE([HAVE_WORKING_RL_COMPENTRY], [1],
              [Does system provide an rl_compentry_func_t that is usable])
  ])
  CXXFLAGS="${save_CXXFLAGS}"
  AC_LANG_POP()
])


AC_DEFUN([PANDORA_CHECK_RL_COMPLETION_FUNC], [
  AC_CACHE_CHECK([defined rl_completion_func_t], [pandora_cv_rl_completion],[
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[
#include "stdio.h"
#include "readline/readline.h"
      ]],[[
rl_completion_func_t *func1= (rl_completion_func_t*)0;
      ]])
    ],[
      pandora_cv_rl_completion=yes
    ],[
      pandora_cv_rl_completion=no
    ])
  ])
  AS_IF([test "$pandora_cv_rl_completion" = "yes"],[
    AC_DEFINE([HAVE_RL_COMPLETION], [1],
              [Does system provide rl_completion_func_t])
  ])
])

AC_DEFUN([_PANDORA_SEARCH_LIBREADLINE], [

  save_LIBS="${LIBS}"
  LIBS=""

  AC_CACHE_CHECK([for a readline compatible library],
                 ac_cv_libreadline, [
    ORIG_LIBS="$LIBS"
    for readline_lib in readline edit editline; do
      for termcap_lib in "" termcap curses ncurses; do
        if test -z "$termcap_lib"; then
          TRY_LIB="-l$readline_lib"
        else
          TRY_LIB="-l$readline_lib -l$termcap_lib"
        fi
        LIBS="$ORIG_LIBS $TRY_LIB"
        AC_TRY_LINK_FUNC(readline, ac_cv_libreadline="$TRY_LIB")
        if test -n "$ac_cv_libreadline"; then
          break
        fi
      done
      if test -n "$ac_cv_libreadline"; then
        break
      fi
    done
    if test -z "$ac_cv_libreadline"; then
      ac_cv_libreadline="no"
      LIBS="$ORIG_LIBS"
    fi
  ])

  if test "$ac_cv_libreadline" != "no"; then
    AC_DEFINE(HAVE_LIBREADLINE, 1,
              [Define if you have a readline compatible library])
    AC_CHECK_HEADERS(readline.h readline/readline.h)
    AC_CACHE_CHECK([whether readline supports history],
                   ac_cv_libreadline_history, [
      ac_cv_libreadline_history="no"
      AC_TRY_LINK_FUNC(add_history, ac_cv_libreadline_history="yes")
    ])
    if test "$ac_cv_libreadline_history" = "yes"; then
      AC_DEFINE(HAVE_READLINE_HISTORY, 1,
                [Define if your readline library has \`add_history'])
      AC_CHECK_HEADERS(history.h readline/history.h)
    fi
  fi
  PANDORA_CHECK_RL_COMPENTRY  
  PANDORA_CHECK_RL_COMPLETION_FUNC
  PANDORA_CHECK_TIOCGWINSZ


  READLINE_LIBS="${LIBS}"
  LIBS="${save_LIBS}"
  AC_SUBST(READLINE_LIBS)

  AM_CONDITIONAL(HAVE_LIBREADLINE, [test "x${ac_cv_libreadline}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBREADLINE],[

  AC_ARG_ENABLE([libreadline],
    [AS_HELP_STRING([--disable-libreadline],
      [Build with libreadline support @<:@default=on@:>@])],
    [ac_enable_libreadline="$enableval"],
    [ac_enable_libreadline="yes"])

  _PANDORA_SEARCH_LIBREADLINE
])


AC_DEFUN([PANDORA_HAVE_LIBREADLINE],[
  AC_REQUIRE([_PANDORA_HAVE_LIBREADLINE])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBREADLINE],[
  ac_enable_libreadline="yes"
  _PANDORA_SEARCH_LIBREADLINE

  AS_IF([test "x$ac_cv_libreadline" = "xno"],
    AC_MSG_ERROR([libreadline is required for ${PACKAGE}. On Debian this can be found in libreadline5-dev. On RedHat this can be found in readline-devel.]))

])

AC_DEFUN([PANDORA_REQUIRE_LIBREADLINE],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBREADLINE])
])


