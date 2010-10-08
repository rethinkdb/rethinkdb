#!/usr/bin/env bash
# libmemcached
# Copyright (C) 2006-2010 Brian Aker, Monty Taylor, Trond Norbye
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.

AC_PREREQ(2.59)
AC_INIT([libmemcached],[0.44],[http://libmemcached.org/])
AC_CONFIG_SRCDIR([libmemcached/memcached.c])
AC_CONFIG_AUX_DIR(config)

PANDORA_CANONICAL_TARGET

#shared library versioning
MEMCACHED_UTIL_LIBRARY_VERSION=1:0:0
MEMCACHED_PROTOCAL_LIBRARY_VERSION=0:0:0
MEMCACHED_LIBRARY_VERSION=6:0:0
#                         | | |
#                  +------+ | +---+
#                  |        |     |
#                 current:revision:age
#                  |        |     |
#                  |        |     +- increment if interfaces have been added
#                  |        |        set to zero if interfaces have been removed or changed
#                  |        +- increment if source code has changed
#                  |           set to zero if current is incremented
#                  +- increment if interfaces have been added, removed or changed
AC_SUBST(MEMCACHED_UTIL_LIBRARY_VERSION)
AC_SUBST(MEMCACHED_PROTOCAL_LIBRARY_VERSION)
AC_SUBST(MEMCACHED_LIBRARY_VERSION)


HASHKIT_LIBRARY_VERSION=0:0:0
AC_SUBST(HASHKIT_LIBRARY_VERSION)

AH_TOP([
#ifndef CONFIG_H
#define CONFIG_H

#ifdef _SYS_FEATURE_TESTS_H
#error "You should include config.h as your first include file"
#endif

#ifdef WIN32
#define _WIN32_WINNT 0x0501
#endif
])

AH_BOTTOM([
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#else
#include "poll/poll.h"
#endif

/* To hide the platform differences between MS Windows and Unix, I am
 * going to use the Microsoft way and #define the Microsoft-specific
 * functions to the unix way. Microsoft use a separate subsystem for sockets,
 * but Unix normally just use a filedescriptor on the same functions. It is
 * a lot easier to map back to the unix way with macros than going the other
 * way without side effect ;-)
 */
#ifdef WIN32
#include "win32/wrappers.h"
#define get_socket_errno() WSAGetLastError()
#else
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(a) close(a)
#define get_socket_errno() errno
#endif

#endif
])

AC_SEARCH_LIBS(getopt_long, gnugetopt)
AC_SEARCH_LIBS(gethostbyname, nsl)

AC_CHECK_FUNCS([getline])

PANDORA_HAVE_LIBGTEST
PANDORA_HAVE_LIBEVENT
my_saved_libs="$LIBS"
LIBS=
PANDORA_REQUIRE_PTHREAD
LIBS="$my_saved_libs"
PANDORA_CXX_DEMANGLE

dnl Specialty checks
DETECT_BYTEORDER
ENABLE_UTILLIB
SETSOCKOPT_SANITY
ENABLE_HSIEH_HASH
PROTOCOL_BINARY_TEST
WITH_MEMCACHED
ENABLE_DEPRECATED
PANDORA_HAVE_LIBINNODB
PANDORA_PRINT_CALLSTACK
PANDORA_HAVE_SASL

dnl The sasl functions should only be visible if we build with sasl support
AS_IF([test "x$ac_cv_sasl" = "xyes"],
      [LIBMEMCACHED_WITH_SASL_SUPPORT="#define LIBMEMCACHED_WITH_SASL_SUPPORT 1"])
AC_SUBST(LIBMEMCACHED_WITH_SASL_SUPPORT)

AC_CHECK_HEADERS([atomic.h])
AS_IF([test "x$ac_cv_header_atomic_h" = "xyes"],[
      AC_CHECK_FUNCS(atomic_add_64)
      AC_CHECK_FUNCS(atomic_add_32)
      AS_IF([test "x$ac_cv_func_atomic_add_64" = "xyes" -a "x$ac_cv_func_atomic_add_32" = "xyes"],[
            AC_DEFINE([USE_ATOMIC_H],
	              [1],
                      [Define to true if you want to use functions from atomic.h])])])

AC_ARG_WITH([docs],
  [AS_HELP_STRING([--with-docs],
    [Generate documentation (yes|no) @<:@default=yes@:>@])],
  [with_docs=$withval],
  [with_docs=yes])

AS_IF([test "$with_docs" = "yes"],
  [
    REQUIRE_POD2MAN
    REQUIRE_PODCHECKER
  ])
AM_CONDITIONAL(BUILD_DOCS, test "$with_docs" = "yes")

AC_CHECK_HEADERS_ONCE(winsock2.h poll.h sys/wait.h fnmatch.h)
AM_CONDITIONAL(BUILD_POLL, test "x$ac_cv_header_poll_h" = "xno")
AM_CONDITIONAL(BUILD_WIN32_WRAPPERS, test "x$ac_cv_header_winsock2_h" = "xyes")
AS_IF(test "x$ac_cv_header_winsock2_h" = "xyes",
      [AM_LDFLAGS="$AM_LDFLAGS -lws2_32"
       AM_CFLAGS="$AM_CFLAGS $NO_WERROR"
       AM_CXXFLAGS="$AM_CXXFLAGS $NO_WERROR"
      ])
DETECT_EAGAIN

AC_CONFIG_FILES([
  Makefile
  docs/Makefile
  libhashkit/configure.h
  libmemcached/configure.h
  support/libmemcached.pc
  support/libmemcached.spec
  support/libmemcached-fc.spec
  ])
AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $with_debug"
echo "   * Warnings as failure:       $ac_cv_warnings_as_errors"
echo ""
echo "---"

case "$host_os" in
  *freebsd*)
    echo "*****"
    echo "*"
    echo "*  NOTE: You are on FreeBSD. BSD make will not work."
    echo "*"
    echo "*   use 'gmake' To build libmemcached"
    echo "*"
    echo "*****"
  ;;
esac

AS_IF(test "$with_docs" = "no",
  [
    echo "*****"
    echo "*"
    echo "*  WARNING: You are not generating any documentation."
    echo "*           Please don't ship libmemcached to an end user"
    echo "*           without documentation..."
    echo "*"
    echo "*****"
  ])
