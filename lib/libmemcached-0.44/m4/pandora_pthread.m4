dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl   
dnl pandora-build: A pedantic build system
dnl Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl
dnl From Monty Taylor


dnl --------------------------------------------------------------------
dnl  Check for libpthread
dnl --------------------------------------------------------------------

AC_DEFUN([PANDORA_PTHREAD_YIELD],[
  AC_REQUIRE([ACX_PTHREAD])

  save_CFLAGS="${CFLAGS}"
  save_CXXFLAGS="${CXXFLAGS}"
  CFLAGS="${PTHREAD_CFLAGS} ${CFLAGS}"
  CXXFLAGS="${PTHREAD_CFLAGS} ${CXXFLAGS}"
  dnl Some OSes like Mac OS X have that as a replacement for pthread_yield()
  AC_CHECK_FUNCS(pthread_yield_np)
  AC_CACHE_CHECK([if pthread_yield takes zero arguments],
    [pandora_cv_pthread_yield_zero_arg],
    [AC_LINK_IFELSE([
      AC_LANG_PROGRAM([[
#include <pthread.h>
        ]],[[
  pthread_yield();
        ]])],
      [pandora_cv_pthread_yield_zero_arg=yes],
      [pandora_cv_pthread_yield_zero_arg=no])])
  AS_IF([test "$pandora_cv_pthread_yield_zero_arg" = "yes"],[
    AC_DEFINE([HAVE_PTHREAD_YIELD_ZERO_ARG], [1],
              [pthread_yield that doesn't take any arguments])
  ])

  AC_CACHE_CHECK([if pthread_yield takes one argument],
    [pandora_cv_pthread_yield_one_arg],
    [AC_LINK_IFELSE([
      AC_LANG_PROGRAM([[
#include <pthread.h>
        ]],[[
  pthread_yield(0);
        ]])],
      [pandora_cv_pthread_yield_one_arg=yes],
      [pandora_cv_pthread_yield_one_arg=no])])
  AS_IF([test "$pandora_cv_pthread_yield_one_arg" = "yes"],[
    AC_DEFINE([HAVE_PTHREAD_YIELD_ONE_ARG], [1],
              [pthread_yield function with one argument])
  ])

  AC_CHECK_FUNCS(pthread_attr_getstacksize pthread_attr_setprio \
    pthread_attr_setschedparam \
    pthread_attr_setstacksize pthread_condattr_create pthread_getsequence_np \
    pthread_key_delete pthread_rwlock_rdlock pthread_setprio \
    pthread_setprio_np pthread_setschedparam pthread_sigmask \
    pthread_attr_create rwlock_init
)



# Check definition of pthread_getspecific
AC_CACHE_CHECK([args to pthread_getspecific], [pandora_cv_getspecific_args],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if !defined(_REENTRANT)
#define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS 
#define _POSIX_PTHREAD_SEMANTICS 
#endif
#include <pthread.h>
   ]], [[
void *pthread_getspecific(pthread_key_t key);
pthread_getspecific((pthread_key_t) NULL);
   ]])],
    [pandora_cv_getspecific_args=POSIX],
    [pandora_cv_getspecific_args=other])])
  if test "$pandora_cv_getspecific_args" = "other"
  then
    AC_DEFINE([HAVE_NONPOSIX_PTHREAD_GETSPECIFIC], [1],
              [For some non posix threads])
  fi

  # Check definition of pthread_mutex_init
  AC_CACHE_CHECK([args to pthread_mutex_init], [pandora_cv_mutex_init_args],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 
#endif
#include <pthread.h> ]], [[ 
  pthread_mutexattr_t attr;
  pthread_mutex_t mp;
  pthread_mutex_init(&mp,&attr); ]])],
      [pandora_cv_mutex_init_args=POSIX],
      [pandora_cv_mutex_init_args=other])])
  if test "$pandora_cv_mutex_init_args" = "other"
  then
    AC_DEFINE([HAVE_NONPOSIX_PTHREAD_MUTEX_INIT], [1],
              [For some non posix threads])
  fi
#---END:

#---START: Used in for client configure
# Check definition of readdir_r
AC_CACHE_CHECK([args to readdir_r], [pandora_cv_readdir_r],
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS 
#define _POSIX_PTHREAD_SEMANTICS 
#endif
#include <pthread.h>
#include <dirent.h>]], [[ int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
readdir_r((DIR *) NULL, (struct dirent *) NULL, (struct dirent **) NULL); ]])],
    [pandora_cv_readdir_r=POSIX],
    [pandora_cv_readdir_r=other])])
if test "$pandora_cv_readdir_r" = "POSIX"
then
  AC_DEFINE([HAVE_READDIR_R], [1], [POSIX readdir_r])
fi

# Check definition of posix sigwait()
AC_CACHE_CHECK([style of sigwait], [pandora_cv_sigwait],
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 
#endif
#include <pthread.h>
#include <signal.h>
      ]], [[
#ifndef _AIX
sigset_t set;
int sig;
sigwait(&set,&sig);
#endif
      ]])],
    [pandora_cv_sigwait=POSIX],
    [pandora_cv_sigwait=other])])
if test "$pandora_cv_sigwait" = "POSIX"
then
  AC_DEFINE([HAVE_SIGWAIT], [1], [POSIX sigwait])
fi

if test "$pandora_cv_sigwait" != "POSIX"
then
unset pandora_cv_sigwait
# Check definition of posix sigwait()
AC_CACHE_CHECK([style of sigwait], [pandora_cv_sigwait],
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 
#endif
#include <pthread.h>
#include <signal.h>
      ]], [[
sigset_t set;
int sig;
sigwait(&set);
      ]])],
    [pandora_cv_sigwait=NONPOSIX],
    [pandora_cv_sigwait=other])])
if test "$pandora_cv_sigwait" = "NONPOSIX"
then
  AC_DEFINE([HAVE_NONPOSIX_SIGWAIT], [1], [sigwait with one argument])
fi
fi
#---END:

# Check if pthread_attr_setscope() exists
AC_CACHE_CHECK([for pthread_attr_setscope], [pandora_cv_pthread_attr_setscope],
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#ifndef _REENTRANT
#define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 
#endif
#include <pthread.h>
      ]], [[
pthread_attr_t thr_attr;
pthread_attr_setscope(&thr_attr,0);
      ]])],
    [pandora_cv_pthread_attr_setscope=yes],
    [pandora_cv_pthread_attr_setscope=no])])
if test "$pandora_cv_pthread_attr_setscope" = "yes"
then
  AC_DEFINE([HAVE_PTHREAD_ATTR_SETSCOPE], [1], [pthread_attr_setscope])
fi


AC_CACHE_CHECK([if pthread_yield takes zero arguments], ac_cv_pthread_yield_zero_arg,
[AC_TRY_LINK([#define _GNU_SOURCE
#include <pthread.h>
#ifdef __cplusplus
extern "C"
#endif
],
[
  pthread_yield();
], ac_cv_pthread_yield_zero_arg=yes, ac_cv_pthread_yield_zero_arg=yeso)])
if test "$ac_cv_pthread_yield_zero_arg" = "yes"
then
  AC_DEFINE([HAVE_PTHREAD_YIELD_ZERO_ARG], [1],
            [pthread_yield that doesn't take any arguments])
fi
AC_CACHE_CHECK([if pthread_yield takes 1 argument], ac_cv_pthread_yield_one_arg,
[AC_TRY_LINK([#define _GNU_SOURCE
#include <pthread.h>
#ifdef __cplusplus
extern "C"
#endif
],
[
  pthread_yield(0);
], ac_cv_pthread_yield_one_arg=yes, ac_cv_pthread_yield_one_arg=no)])
if test "$ac_cv_pthread_yield_one_arg" = "yes"
then
  AC_DEFINE([HAVE_PTHREAD_YIELD_ONE_ARG], [1],
            [pthread_yield function with one argument])
fi

  CFLAGS="${save_CFLAGS}"
  CXXFLAGS="${save_CXXFLAGS}"
])


AC_DEFUN([_PANDORA_SEARCH_PTHREAD],[
  AC_REQUIRE([ACX_PTHREAD])
  LIBS="${PTHREAD_LIBS} ${LIBS}"
  AM_CFLAGS="${PTHREAD_CFLAGS} ${AM_CFLAGS}"
  AM_CXXFLAGS="${PTHREAD_CFLAGS} ${AM_CXXFLAGS}"
  PANDORA_PTHREAD_YIELD
])


AC_DEFUN([PANDORA_HAVE_PTHREAD],[
  AC_REQUIRE([_PANDORA_SEARCH_PTHREAD])
])

AC_DEFUN([PANDORA_REQUIRE_PTHREAD],[
  AC_REQUIRE([PANDORA_HAVE_PTHREAD])
  AS_IF([test "x$acx_pthread_ok" != "xyes"],[
    AC_MSG_ERROR(could not find libpthread)])
])
