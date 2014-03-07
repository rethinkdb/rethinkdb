# Copyright (c) 1999-2001 by Red Hat, Inc. All rights reserved.
# 
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
# OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
# 
# Permission is hereby granted to use or copy this program
# for any purpose,  provided the above notices are retained on all copies.
# Permission to modify the code and to distribute modified code is granted,
# provided the above notices are retained, and a notice that the code was
# modified is included with the above copyright notice.
#
# Original author: Tom Tromey
# Modified by: Grzegorz Jakacki <jakacki at acm dot org>

dnl Process this file with autoconf to produce configure.

# Initialization
# ==============

AC_INIT(gc,7.0,Hans.Boehm@hp.com) 
    ## version must conform to [0-9]+[.][0-9]+(alpha[0-9]+)?
AC_CONFIG_SRCDIR(gcj_mlc.c)
AC_CANONICAL_TARGET 
AC_PREREQ(2.53)
AC_REVISION($Revision: 1.25 $)
GC_SET_VERSION
AM_INIT_AUTOMAKE([foreign dist-bzip2 subdir-objects nostdinc])
AM_MAINTAINER_MODE

AC_SUBST(PACKAGE)
AC_SUBST(GC_VERSION)

AM_PROG_CC_C_O
AC_PROG_CXX

AM_PROG_AS
## FIXME: really needed? (AC_LIBTOOL already provides this)
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib, :)  # :)

AC_PROG_INSTALL

. ${srcdir}/configure.host

GC_CFLAGS=${gc_cflags}
AC_SUBST(GC_CFLAGS)

AC_ARG_ENABLE(threads,
  [AC_HELP_STRING([--enable-threads=TYPE], [choose threading package])],
  THREADS=$enableval,
  [ AC_MSG_CHECKING([for thread model used by GCC])
    THREADS=`$CC -v 2>&1 | sed -n 's/^Thread model: //p'`
    if test -z "$THREADS"; then
      THREADS=no
    fi
    AC_MSG_RESULT([$THREADS]) ])

AC_ARG_ENABLE(parallel-mark,
   [AC_HELP_STRING([--enable-parallel-mark],
	[parallelize marking and free list construction])],
   [case "$THREADS" in
      no | none | single)
	AC_MSG_ERROR([Parallel mark requires --enable-threads=x spec])
	;;
    esac ]
)

AC_ARG_ENABLE(cplusplus,
    [AC_HELP_STRING([--enable-cplusplus], [install C++ support])])

INCLUDES=-I${srcdir}/include
THREADDLLIBS=
need_atomic_ops_asm=false
## Libraries needed to support dynamic loading and/or threads.
case "$THREADS" in
 no | none | single)
    THREADS=none
    ;;
 posix | pthreads)
    THREADS=posix
    THREADDLLIBS=-lpthread
    case "$host" in
     x86-*-linux* | ia64-*-linux* | i586-*-linux* | i686-*-linux* | x86_64-*-linux* | alpha-*-linux*)
	AC_DEFINE(GC_LINUX_THREADS)
	AC_DEFINE(_REENTRANT)
        if test "${enable_parallel_mark}" = yes; then
	  AC_DEFINE(PARALLEL_MARK)
	fi
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	AC_MSG_WARN("Explict GC_INIT() calls may be required.");
	;;
     *-*-linux*)
	AC_DEFINE(GC_LINUX_THREADS)
	AC_DEFINE(_REENTRANT)
	;;
     *-*-aix*)
	AC_DEFINE(GC_AIX_THREADS)
	AC_DEFINE(_REENTRANT)
	;;
     *-*-hpux11*)
	AC_MSG_WARN("Only HP/UX 11 POSIX threads are supported.")
	AC_DEFINE(GC_HPUX_THREADS)
	AC_DEFINE(_POSIX_C_SOURCE,199506L)
	if test "${enable_parallel_mark}" = yes; then
	  AC_DEFINE(PARALLEL_MARK)
	fi
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	AC_MSG_WARN("Explict GC_INIT() calls may be required.");
	THREADDLLIBS="-lpthread -lrt"
	# HPUX needs REENTRANT for the _r calls.
	AC_DEFINE(_REENTRANT, 1, [Required define if using POSIX threads])
	;;
     *-*-hpux10*)
	AC_MSG_WARN("Only HP-UX 11 POSIX threads are supported.")
	;;
     *-*-freebsd*)
	AC_MSG_WARN("FreeBSD does not yet fully support threads with Boehm GC.")
	AC_DEFINE(GC_FREEBSD_THREADS)
	INCLUDES="$INCLUDES -pthread"
      	;;
     *-*-kfreebsd*-gnu)
	AC_DEFINE(GC_FREEBSD_THREADS)
	INCLUDES="$INCLUDES -pthread"
	THREADDLLIBS=-pthread
	AC_DEFINE(_REENTRANT)
        if test "${enable_parallel_mark}" = yes; then
	  AC_DEFINE(PARALLEL_MARK)
	fi
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	AC_DEFINE(USE_COMPILER_TLS)
      	;;
     *-*-gnu*)
	AC_DEFINE(GC_GNU_THREADS)
	AC_DEFINE(_REENTRANT)
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	;;
     *-*-netbsd*)
	AC_MSG_WARN("Only on NetBSD 2.0 or later.")
	AC_DEFINE(GC_NETBSD_THREADS)
	AC_DEFINE(_REENTRANT)
	AC_DEFINE(_PTHREADS)
	THREADDLLIBS="-lpthread -lrt"
	;;
     *-*-solaris*)
	AC_DEFINE(GC_SOLARIS_THREADS)
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	THREADDLLIBS="-lpthread -lrt"
	if test "$GCC" != yes; then
          CFLAGS="$CFLAGS -O"
          need_atomic_ops_asm=true
        fi
	;;
     *-*-irix*)
	AC_DEFINE(GC_IRIX_THREADS)
	;;
     *-*-cygwin*)
	AC_DEFINE(GC_WIN32_THREADS)
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	win32_threads=true
	;;
     *-*-darwin*)
	AC_DEFINE(GC_DARWIN_THREADS)
	AC_DEFINE(THREAD_LOCAL_ALLOC)
	AC_MSG_WARN("Explict GC_INIT() calls may be required.");
	if test "${enable_parallel_mark}" = yes; then
	  AC_DEFINE(PARALLEL_MARK)
	fi
	darwin_threads=true
	;;
     *-*-osf*)
	AC_DEFINE(GC_OSF1_THREADS)
        if test "${enable_parallel_mark}" = yes; then
	  AC_DEFINE(PARALLEL_MARK)
	  AC_DEFINE(THREAD_LOCAL_ALLOC)
	  AC_MSG_WARN("Explict GC_INIT() calls may be required.");
	  # May want to enable it in other cases, too.
	  # Measurements havent yet been done.
	fi
	INCLUDES="$INCLUDES -pthread"
	THREADDLLIBS="-lpthread -lrt"
	;;
      *)
	AC_MSG_ERROR("Pthreads not supported by the GC on this platform.")
	;;
    esac
    ;;
 win32)
    AC_DEFINE(GC_WIN32_THREADS)
    dnl Wine getenv may not return NULL for missing entry
    AC_DEFINE(NO_GETENV)
    ;;
 dgux386)
    THREADS=dgux386
    AC_MSG_RESULT($THREADDLLIBS)
    # Use pthread GCC  switch
    THREADDLLIBS=-pthread
    if test "${enable_parallel_mark}" = yes; then
        AC_DEFINE(PARALLEL_MARK)
    fi
    AC_DEFINE(THREAD_LOCAL_ALLOC)
    AC_MSG_WARN("Explict GC_INIT() calls may be required.");
    AC_DEFINE(GC_DGUX386_THREADS)
    AC_DEFINE(DGUX_THREADS)
    # Enable _POSIX4A_DRAFT10_SOURCE with flag -pthread
    INCLUDES="-pthread $INCLUDES"
    ;;
 aix)
    THREADS=posix
    THREADDLLIBS=-lpthread
    AC_DEFINE(GC_AIX_THREADS)
    AC_DEFINE(_REENTRANT)
    ;;
 decosf1 | irix | mach | os2 | solaris | dce | vxworks)
    AC_MSG_ERROR(thread package $THREADS not yet supported)
    ;;
 *)
    AC_MSG_ERROR($THREADS is an unknown thread package)
    ;;
esac
AC_SUBST(THREADDLLIBS)
AM_CONDITIONAL(THREADS, test x$THREADS != xnone)
AM_CONDITIONAL(PTHREADS, test x$THREADS = xposix)
AM_CONDITIONAL(DARWIN_THREADS, test x$darwin_threads = xtrue)
AM_CONDITIONAL(WIN32_THREADS, test x$win32_threads = xtrue)

case "$host" in 
   powerpc-*-darwin*)
      powerpc_darwin=true
      ;;
esac

# Darwin needs a few extra special tests to deal with variation in the
# system headers.
case "$host" in
  powerpc*-*-darwin*)
    AC_CHECK_MEMBER(ppc_thread_state_t.r0,
      AC_DEFINE(HAS_PPC_THREAD_STATE_R0,1,
	[ppc_thread_state_t has field r0]),,
      [#include <mach/thread_status.h>])
    AC_CHECK_MEMBER(ppc_thread_state_t.__r0,
      AC_DEFINE(HAS_PPC_THREAD_STATE___R0,1,dnl
	[ppc_thread_state_t has field __r0]),,
      [#include <mach/thread_status.h>])
    AC_CHECK_MEMBER(ppc_thread_state64_t.r0,
      AC_DEFINE(HAS_PPC_THREAD_STATE64_R0,1,dnl
	[ppc_thread_state64_t has field r0]),,
      [#include <mach/thread_status.h>])
    AC_CHECK_MEMBER(ppc_thread_state64_t.__r0,
      AC_DEFINE(HAS_PPC_THREAD_STATE64___R0,1,dnl
	[ppc_thread_state64_t has field __r0]),,
      [#include <mach/thread_status.h>])
    ;;
  i?86*-*-darwin*)
    AC_CHECK_MEMBER(x86_thread_state32_t.eax,
      AC_DEFINE(HAS_X86_THREAD_STATE32_EAX,1,dnl
	[x86_thread_state32_t has field eax]),,
      [#include <sys/cdefs.h>
      #include <mach/thread_status.h>])
    AC_CHECK_MEMBER(x86_thread_state32_t.__eax,
      AC_DEFINE(HAS_X86_THREAD_STATE32___EAX,1,dnl
	[x86_thread_state32_t has field __eax]),,
      [#include <sys/cdefs.h>
      #include <mach/thread_status.h>])
    ;;
  x86_64-*-darwin*)
    AC_CHECK_MEMBER(x86_thread_state64_t.rax,
      AC_DEFINE(HAS_X86_THREAD_STATE64_RAX,1,dnl
	[x86_thread_state64_t has field rax]),,
      [#include <sys/cdefs.h>
      #include <mach/thread_status.h>])
    AC_CHECK_MEMBER(x86_thread_state64_t.__rax,
      AC_DEFINE(HAS_X86_THREAD_STATE64___RAX,1,dnl
	[x86_thread_state64_t has field __rax]),,
      [#include <sys/cdefs.h>
      #include <mach/thread_status.h>])
     ;;
  *) ;;
esac

AC_MSG_CHECKING(for xlc)
AC_TRY_COMPILE([],[
 #ifndef __xlC__
 # error
 #endif
], [compiler_xlc=yes], [compiler_xlc=no])
AC_MSG_RESULT($compiler_xlc)
AM_CONDITIONAL(COMPILER_XLC,test $compiler_xlc = yes)
if test $compiler_xlc = yes -a "$powerpc_darwin" = true; then
  # the darwin stack-frame-walking code is completely broken on xlc
  AC_DEFINE(DARWIN_DONT_PARSE_STACK)
fi

# We never want libdl on darwin. It is a fake libdl that just ends up making
# dyld calls anyway
case "$host" in
  *-*-darwin*) ;;
  *)
    AC_CHECK_LIB(dl, dlopen, THREADDLLIBS="$THREADDLLIBS -ldl")
    ;;
esac

# extra LD Flags which are required for targets
case "${host}" in
  *-*-darwin*)
    extra_ldflags_libgc=-Wl,-single_module
    ;;
esac
AC_SUBST(extra_ldflags_libgc)

AC_SUBST(EXTRA_TEST_LIBS)

target_all=libgc.la
AC_SUBST(target_all)

dnl If the target is an eCos system, use the appropriate eCos
dnl I/O routines.
dnl FIXME: this should not be a local option but a global target
dnl system; at present there is no eCos target.
TARGET_ECOS="no"
AC_ARG_WITH(ecos,
[  --with-ecos             enable runtime eCos target support],
TARGET_ECOS="$with_ecos"
)

addobjs=
addlibs=
CXXINCLUDES=
case "$TARGET_ECOS" in
   no)
      ;;
   *)
      AC_DEFINE(ECOS)
      CXXINCLUDES="-I${TARGET_ECOS}/include"
      addobjs="$addobjs ecos.lo"
      ;;
esac

AM_CONDITIONAL(CPLUSPLUS, test "${enable_cplusplus}" = yes)

AC_SUBST(CXX)

AC_SUBST(INCLUDES)
AC_SUBST(CXXINCLUDES)

# Configuration of shared libraries
#
AC_MSG_CHECKING(whether to build shared libraries)
AC_ENABLE_SHARED

case "$host" in
 alpha-*-openbsd*)
     enable_shared=no
     AC_MSG_RESULT(no)
     ;;
 *)
     AC_MSG_RESULT(yes)
     ;;
esac

# Configuration of machine-dependent code
#
AC_MSG_CHECKING(which machine-dependent code should be used) 
machdep=
case "$host" in
 alpha-*-openbsd*)
    machdep="mach_dep.lo"
    if test x"${ac_cv_lib_dl_dlopen}" != xyes ; then
       AC_MSG_WARN(OpenBSD/Alpha without dlopen(). Shared library support is disabled)
    fi
    ;;
 alpha*-*-linux*)
    machdep="mach_dep.lo"
    ;;
 i?86-*-solaris2.[[89]] | i?86-*-solaris2.1?)
    AC_DEFINE(SOLARIS25_PROC_VDB_BUG_FIXED)
    ;;
 mipstx39-*-elf*)
    machdep="mach_dep.lo"
    AC_DEFINE(STACKBASE, __stackbase)
    AC_DEFINE(DATASTART_IS_ETEXT)
    ;;
 mips-dec-ultrix*)
    machdep="mach-dep.lo"
    ;;
 mips-nec-sysv*|mips-unknown-sysv*)
    ;;
 mips*-*-linux*) 
    ;; 
 mips-*-*)
    machdep="mach_dep.lo"
    dnl AC_DEFINE(NO_EXECUTE_PERMISSION)
    dnl This is now redundant, but it is also important for incremental GC
    dnl performance under Irix.
    ;;
 sparc-*-netbsd*)
    machdep="mach_dep.lo sparc_netbsd_mach_dep.lo"
    ;;
 sparc-sun-solaris2.3)
    machdep="mach_dep.lo sparc_mach_dep.lo"
    AC_DEFINE(SUNOS53_SHARED_LIB)
    ;;
 sparc*-sun-solaris2.*)
    machdep="mach_dep.lo sparc_mach_dep.lo"
    ;;
 ia64-*-*)
    machdep="mach_dep.lo ia64_save_regs_in_stack.lo"
    ;;
esac
if test x"$machdep" = x; then
AC_MSG_RESULT($machdep)
   machdep="mach_dep.lo"
fi
addobjs="$addobjs $machdep"
AC_SUBST(addobjs)
AC_SUBST(addlibs)

AC_PROG_LIBTOOL

#
# Check for AViiON Machines running DGUX
#
ac_is_dgux=no
AC_CHECK_HEADER(sys/dg_sys_info.h,
[ac_is_dgux=yes;])

    ## :GOTCHA: we do not check anything but sys/dg_sys_info.h
if test $ac_is_dgux = yes; then
    if test "$enable_full_debug" = "yes"; then
      CFLAGS="-g -mstandard -DDGUX -D_DGUX_SOURCE -Di386 -mno-legend -O2"
      CXXFLAGS="-g -mstandard -DDGUX -D_DGUX_SOURCE -Di386 -mno-legend -O2"
    else
      CFLAGS="-DDGUX -D_DGUX_SOURCE -Di386 -mno-legend -O2"
      CXXFLAGS="-DDGUX -D_DGUX_SOURCE -Di386 -mno-legend -O2"
    fi
    AC_SUBST(CFLAGS)
    AC_SUBST(CXXFLAGS)
fi

dnl We use these options to decide which functions to include.
AC_ARG_WITH(target-subdir,
[  --with-target-subdir=SUBDIR
                          configuring with a cross compiler])
AC_ARG_WITH(cross-host,
[  --with-cross-host=HOST  configuring with a cross compiler])

# automake wants to see AC_EXEEXT.  But we don't need it.  And having
# it is actually a problem, because the compiler we're passed can't
# necessarily do a full link.  So we fool automake here.
if false; then
  # autoconf 2.50 runs AC_EXEEXT by default, and the macro expands
  # to nothing, so nothing would remain between `then' and `fi' if it
  # were not for the `:' below.
  :
  AC_EXEEXT
fi

dnl As of 4.13a2, the collector will not properly work on Solaris when
dnl built with gcc and -O.  So we remove -O in the appropriate case.
dnl Not needed anymore on Solaris.
AC_MSG_CHECKING(whether Solaris gcc optimization fix is necessary)
case "$host" in
 *aix*)
    if test "$GCC" = yes; then
       AC_MSG_RESULT(yes)
       new_CFLAGS=
       for i in $CFLAGS; do
	  case "$i" in
	   -O*)
	      ;;
	   *)
	      new_CFLAGS="$new_CFLAGS $i"
	      ;;
	  esac
       done
       CFLAGS="$new_CFLAGS"
    else
       AC_MSG_RESULT(no)
    fi
    ;;
 *) AC_MSG_RESULT(no) ;;
esac

dnl Include defines that have become de facto standard.
dnl ALL_INTERIOR_POINTERS can be overridden in startup code.
AC_DEFINE(NO_EXECUTE_PERMISSION)
AC_DEFINE(ALL_INTERIOR_POINTERS)


dnl Interface Selection
dnl -------------------
dnl
dnl By default, make the library as general as possible.
dnl enable_gcj_support=no
AC_ARG_ENABLE(gcj-support,
    [AC_HELP_STRING([--disable-gcj-support],
	[Disable support for gcj.])])
AM_CONDITIONAL(ENABLE_GCJ_SUPPORT,
    [test x"$enable_gcj_support" != xno])
if test x"$enable_gcj_support" != xno; then
    AC_DEFINE(GC_GCJ_SUPPORT, 1, [Define to include support for gcj])
fi

AC_ARG_ENABLE(java-finalization,
    [AC_HELP_STRING([--disable-java-finalization],
	[Disable support for java finalization.])])
if test x"$enable_java_finalization" != xno; then
    AC_DEFINE(JAVA_FINALIZATION)
fi

AC_ARG_ENABLE(atomic-uncollectable,
    [AC_HELP_STRING([--disable-atomic-uncollectible],
	[Disable support for atomic uncollectible allocation.])])
if test x"$enable_atomic_uncollectible" != x"no"; then
    AC_DEFINE(ATOMIC_UNCOLLECTABLE, 1,
	[Define to enable atomic uncollectible allocation.])
fi

AC_ARG_ENABLE(redirect-malloc,
    [AC_HELP_STRING([--enable-redirect-malloc],
	[Redirect malloc and friends to GC routines])])

if test "${enable_redirect_malloc}" = yes; then
    if test "${enable_full_debug}" = yes; then
	AC_DEFINE(REDIRECT_MALLOC, GC_debug_malloc_replacement)
	AC_DEFINE(REDIRECT_REALLOC, GC_debug_realloc_replacement)
	AC_DEFINE(REDIRECT_FREE, GC_debug_free)
    else
	AC_DEFINE(REDIRECT_MALLOC, GC_malloc)
    fi
fi

AC_ARG_ENABLE(large-config,
    [AC_HELP_STRING([--enable-large-config],
	[Optimize for large (> 100 MB) heap or root set])])

if test "${enable_large_config}" = yes; then
    AC_DEFINE(LARGE_CONFIG, 1, [Define to optimize for large heaps or root sets])
fi

dnl This is something of a hack.  When cross-compiling we turn off
dnl some functionality.  We also enable the "small" configuration.
dnl These is only correct when targetting an embedded system.  FIXME.
if test -n "${with_cross_host}"; then
   AC_DEFINE(NO_CLOCK)
   AC_DEFINE(SMALL_CONFIG)
   AC_DEFINE(NO_DEBUGGING)
fi


dnl Debugging
dnl ---------

UNWINDLIBS=
AC_ARG_ENABLE(gc-debug,
[AC_HELP_STRING([--enable-gc-debug],
    [include full support for pointer backtracing etc.])],
[ if test "$enable_gc_debug" = "yes"; then
    AC_MSG_WARN("Should define GC_DEBUG and use debug alloc. in clients.")
    AC_DEFINE(KEEP_BACK_PTRS)
    AC_DEFINE(DBG_HDRS_ALL)
    case $host in
      ia64-*-linux* )
	AC_DEFINE(MAKE_BACK_GRAPH)
	AC_DEFINE(SAVE_CALL_COUNT, 8)
        AC_CHECK_LIB(unwind, backtrace, [
	  AC_DEFINE(GC_HAVE_BUILTIN_BACKTRACE)
	  UNWINDLIBS=-lunwind
	  AC_MSG_WARN("Client code may need to link against libunwind.")
	])
      ;;
      x86-*-linux* | i586-*-linux* | i686-*-linux* | x86_64-*-linux* )
	AC_DEFINE(MAKE_BACK_GRAPH)
	AC_MSG_WARN("Client must not use -fomit-frame-pointer.")
	AC_DEFINE(SAVE_CALL_COUNT, 8)
      ;;
      i[3456]86-*-dgux*)
	AC_DEFINE(MAKE_BACK_GRAPH)
      ;;
    esac ]
  fi)

AC_SUBST(UNWINDLIBS)

AC_ARG_ENABLE(gc-assertions,
    [AC_HELP_STRING([--enable-gc-assertions],
	[collector-internal assertion checking])])
if test "${enable_gc_assertions}" = yes; then
    AC_DEFINE(GC_ASSERTIONS)
fi

AM_CONDITIONAL(USE_LIBDIR, test -z "$with_cross_host")


dnl Atomic Ops
dnl ----------

atomic_ops_libs=-latomic_ops
AC_CHECK_HEADER(atomic_ops.h,
  [ AC_MSG_NOTICE([Using pre-installed libatomic_ops]) ],
  [ ao_dir=
    for candidate in ${srcdir}/libatomic_ops*; do
	case $candidate in
	    *.tar.gz)
		;;
	    *install)
	    	dnl generated by alternate Makefile.
		;;
	    *)
		if test -e "$candidate"; then
		    ao_dir="$candidate"
		fi
		;;
	esac
    done
    if test -z "$ao_dir"; then
	AC_MSG_ERROR([Missig libatomic_ops.])
    fi
    ao_version="${ao_dir#*libatomic_ops-}"
    AC_MSG_NOTICE([Using internal version of libatomic_ops])

    dnl Automake does not accept shell variables in AC_CONFIG_SUBDIRS
    test -e ${srcdir}/libatomic_ops \
	|| ln -s ${ao_dir} ${srcdir}/libatomic_ops
    AC_CONFIG_SUBDIRS(libatomic_ops)

    dnl Also copy the source files to be linked in.
    test -e ${srcdir}/atomic_ops.c \
    	|| ln -s ${srcdir}/libatomic_ops/src/atomic_ops.c \
		 ${srcdir}/atomic_ops.c

    test -e ${srcdir}/atomic_ops_sysdeps.S \
    	|| ln -s ${srcdir}/libatomic_ops/src/atomic_ops_sysdeps.S \
		 ${srcdir}/atomic_ops_sysdeps.S

    dnl This gets the source include files, which is often close enough.
    dnl It also makes atomic_ops_sysdeps.S assemble.
    GC_CFLAGS="${GC_CFLAGS} -I \$(top_srcdir)/libatomic_ops/src"
    maybe_libatomic_ops="libatomic_ops"
  ])

AM_CONDITIONAL(USE_INTERNAL_LIBATOMIC_OPS,
		test -n "$maybe_libatomic_ops" -a "$THREADS" != "none")
AM_CONDITIONAL(NEED_ATOMIC_OPS_ASM, test -n "$maybe_libatomic_ops" -a x$need_atomic_ops_asm = xtrue)
AC_SUBST(atomic_ops_libs)

dnl Produce the Files
dnl -----------------

AC_CONFIG_FILES([Makefile bdw-gc.pc])

AC_CONFIG_COMMANDS([default],,
  [ srcdir=${srcdir}
    host=${host}
    CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
    CC="${CC}"
    DEFS="$DEFS" ])

AC_OUTPUT
