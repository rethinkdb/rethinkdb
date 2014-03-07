/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/* This file is ALSO:
 * Copyright 2001-2004 David Abrahams.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * jam.h - includes and globals for jam
 */

#ifndef JAM_H_VP_2003_08_01
#define JAM_H_VP_2003_08_01

#ifdef HAVE_PYTHON
#include <Python.h>
#endif

/* Assume popen support is available unless known otherwise. */
#define HAVE_POPEN 1

/*
 * Windows NT
 */

#ifdef NT

#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#ifndef __MWERKS__
    #include <memory.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define OSMAJOR "NT=true"
#define OSMINOR "OS=NT"
#define OS_NT
#define SPLITPATH ';'
#define MAXLINE (undefined__see_execnt_c)  /* max chars per command line */
#define USE_EXECNT
#define PATH_DELIM '\\'

/* AS400 cross-compile from NT. */

#ifdef AS400
    #undef OSMINOR
    #undef OSMAJOR
    #define OSMAJOR "AS400=true"
    #define OSMINOR "OS=AS400"
    #define OS_AS400
#endif

/* Metrowerks Standard Library on Windows. */

#ifdef __MSL__
    #undef HAVE_POPEN
#endif

#endif  /* #ifdef NT */


/*
 * Windows MingW32
 */

#ifdef MINGW

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define OSMAJOR "MINGW=true"
#define OSMINOR "OS=MINGW"
#define OS_NT
#define SPLITPATH ';'
#define MAXLINE 996  /* max chars per command line */
#define USE_EXECUNIX
#define PATH_DELIM '\\'

#endif  /* #ifdef MINGW */


/*
 * God fearing UNIX.
 */

#ifndef OSMINOR

#define OSMAJOR "UNIX=true"
#define USE_EXECUNIX
#define USE_FILEUNIX
#define PATH_DELIM '/'

#ifdef _AIX
    #define unix
    #define MAXLINE 23552  /* 24k - 1k, max chars per command line */
    #define OSMINOR "OS=AIX"
    #define OS_AIX
    #define NO_VFORK
#endif
#ifdef AMIGA
    #define OSMINOR "OS=AMIGA"
    #define OS_AMIGA
#endif
#ifdef __BEOS__
    #define unix
    #define OSMINOR "OS=BEOS"
    #define OS_BEOS
    #define NO_VFORK
#endif
#ifdef __bsdi__
    #define OSMINOR "OS=BSDI"
    #define OS_BSDI
#endif
#if defined (COHERENT) && defined (_I386)
    #define OSMINOR "OS=COHERENT"
    #define OS_COHERENT
    #define NO_VFORK
#endif
#if defined(__cygwin__) || defined(__CYGWIN__)
    #define OSMINOR "OS=CYGWIN"
    #define OS_CYGWIN
#endif
#if defined(__FreeBSD__) && !defined(__DragonFly__)
    #define OSMINOR "OS=FREEBSD"
    #define OS_FREEBSD
#endif
#ifdef __DragonFly__
    #define OSMINOR "OS=DRAGONFLYBSD"
    #define OS_DRAGONFLYBSD
#endif
#ifdef __DGUX__
    #define OSMINOR "OS=DGUX"
    #define OS_DGUX
#endif
#ifdef __hpux
    #define OSMINOR "OS=HPUX"
    #define OS_HPUX
#endif
#ifdef __OPENNT
    #define unix
    #define OSMINOR "OS=INTERIX"
    #define OS_INTERIX
    #define NO_VFORK
#endif
#ifdef __sgi
    #define OSMINOR "OS=IRIX"
    #define OS_IRIX
    #define NO_VFORK
#endif
#ifdef __ISC
    #define OSMINOR "OS=ISC"
    #define OS_ISC
    #define NO_VFORK
#endif
#ifdef linux
    #define OSMINOR "OS=LINUX"
    #define OS_LINUX
#endif
#ifdef __Lynx__
    #define OSMINOR "OS=LYNX"
    #define OS_LYNX
    #define NO_VFORK
    #define unix
#endif
#ifdef __MACHTEN__
    #define OSMINOR "OS=MACHTEN"
    #define OS_MACHTEN
#endif
#ifdef mpeix
    #define unix
    #define OSMINOR "OS=MPEIX"
    #define OS_MPEIX
    #define NO_VFORK
#endif
#ifdef __MVS__
    #define unix
    #define OSMINOR "OS=MVS"
    #define OS_MVS
#endif
#ifdef _ATT4
    #define OSMINOR "OS=NCR"
    #define OS_NCR
#endif
#ifdef __NetBSD__
    #define unix
    #define OSMINOR "OS=NETBSD"
    #define OS_NETBSD
    #define NO_VFORK
#endif
#ifdef __QNX__
    #define unix
    #ifdef __QNXNTO__
        #define OSMINOR "OS=QNXNTO"
        #define OS_QNXNTO
    #else
        #define OSMINOR "OS=QNX"
        #define OS_QNX
        #define NO_VFORK
        #define MAXLINE 996  /* max chars per command line */
    #endif
#endif
#ifdef NeXT
    #ifdef __APPLE__
        #define OSMINOR "OS=RHAPSODY"
        #define OS_RHAPSODY
    #else
        #define OSMINOR "OS=NEXT"
        #define OS_NEXT
    #endif
#endif
#ifdef __APPLE__
    #define unix
    #define OSMINOR "OS=MACOSX"
    #define OS_MACOSX
#endif
#ifdef __osf__
    #ifndef unix
        #define unix
    #endif
    #define OSMINOR "OS=OSF"
    #define OS_OSF
#endif
#ifdef _SEQUENT_
    #define OSMINOR "OS=PTX"
    #define OS_PTX
#endif
#ifdef M_XENIX
    #define OSMINOR "OS=SCO"
    #define OS_SCO
    #define NO_VFORK
#endif
#ifdef sinix
    #define unix
    #define OSMINOR "OS=SINIX"
    #define OS_SINIX
#endif
#ifdef sun
    #if defined(__svr4__) || defined(__SVR4)
        #define OSMINOR "OS=SOLARIS"
        #define OS_SOLARIS
    #else
        #define OSMINOR "OS=SUNOS"
        #define OS_SUNOS
    #endif
#endif
#ifdef ultrix
    #define OSMINOR "OS=ULTRIX"
    #define OS_ULTRIX
#endif
#ifdef _UNICOS
    #define OSMINOR "OS=UNICOS"
    #define OS_UNICOS
#endif
#if defined(__USLC__) && !defined(M_XENIX)
    #define OSMINOR "OS=UNIXWARE"
    #define OS_UNIXWARE
#endif
#ifdef __OpenBSD__
    #define OSMINOR "OS=OPENBSD"
    #define OS_OPENBSD
    #define unix
#endif
#if defined (__FreeBSD_kernel__) && !defined(__FreeBSD__)
    #define OSMINOR "OS=KFREEBSD"
    #define OS_KFREEBSD
#endif
#ifndef OSMINOR
    #define OSMINOR "OS=UNKNOWN"
#endif

/* All the UNIX includes */

#include <sys/types.h>

#ifndef OS_MPEIX
    #include <sys/file.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef OS_QNX
    #include <memory.h>
#endif

#ifndef OS_ULTRIX
    #include <stdlib.h>
#endif

#if !defined( OS_BSDI         ) && \
    !defined( OS_FREEBSD      ) && \
    !defined( OS_DRAGONFLYBSD ) && \
    !defined( OS_NEXT         ) && \
    !defined( OS_MACHTEN      ) && \
    !defined( OS_MACOSX       ) && \
    !defined( OS_RHAPSODY     ) && \
    !defined( OS_MVS          ) && \
    !defined( OS_OPENBSD      )
    #include <malloc.h>
#endif

#endif  /* #ifndef OSMINOR */


/*
 * OSPLAT definitions - suppressed when it is a one-of-a-kind.
 */

#if defined( _M_PPC      ) || \
    defined( PPC         ) || \
    defined( ppc         ) || \
    defined( __powerpc__ ) || \
    defined( __ppc__     )
    #define OSPLAT "OSPLAT=PPC"
#endif

#if defined( _ALPHA_   ) || \
    defined( __alpha__ )
    #define OSPLAT "OSPLAT=AXP"
#endif

#if defined( _i386_   ) || \
    defined( __i386__ ) || \
    defined( __i386   ) || \
    defined( _M_IX86  )
    #define OSPLAT "OSPLAT=X86"
#endif

#if defined( __ia64__ ) || \
    defined( __IA64__ ) || \
    defined( __ia64   )
    #define OSPLAT "OSPLAT=IA64"
#endif

#if defined( __x86_64__ ) || \
    defined( __amd64__  ) || \
    defined( _M_AMD64   )
    #define OSPLAT "OSPLAT=X86_64"
#endif

#if defined( __sparc__ ) || \
    defined( __sparc   )
    #define OSPLAT "OSPLAT=SPARC"
#endif

#ifdef __mips__
    #define OSPLAT "OSPLAT=MIPS"
#endif

#ifdef __arm__
    #define OSPLAT "OSPLAT=ARM"
#endif

#ifdef __s390__
    #define OSPLAT "OSPLAT=390"
#endif

#ifdef __hppa
    #define OSPLAT "OSPLAT=PARISC"
#endif

#ifndef OSPLAT
    #define OSPLAT ""
#endif


/*
 * Jam implementation misc.
 */

#ifndef MAXLINE
    #define MAXLINE 102400  /* max chars per command line */
#endif

#ifndef EXITOK
    #define EXITOK 0
    #define EXITBAD 1
#endif

#ifndef SPLITPATH
    #define SPLITPATH ':'
#endif

/* You probably do not need to muck with these. */

#define MAXSYM   1024  /* longest symbol in the environment */
#define MAXJPATH 1024  /* longest filename */

#define MAXJOBS  64    /* internally enforced -j limit */
#define MAXARGC  32    /* words in $(JAMSHELL) */

/* Jam private definitions below. */

#define DEBUG_MAX  14


struct globs
{
    int    noexec;
    int    jobs;
    int    quitquick;
    int    newestfirst;         /* build newest sources first */
    int    pipe_action;
    char   debug[ DEBUG_MAX ];
    FILE * cmdout;              /* print cmds, not run them */
    long   timeout;             /* number of seconds to limit actions to,
                                 * default 0 for no limit.
                                 */
    int    dart;                /* output build and test results formatted for
                                 * Dart
                                 */
    int    max_buf;             /* maximum amount of output saved from target
                                 * (kb)
                                 */
};

extern struct globs globs;

#define DEBUG_MAKE     ( globs.debug[ 1 ] )   /* show actions when executed */
#define DEBUG_MAKEQ    ( globs.debug[ 2 ] )   /* show even quiet actions */
#define DEBUG_EXEC     ( globs.debug[ 2 ] )   /* show text of actons */
#define DEBUG_MAKEPROG ( globs.debug[ 3 ] )   /* show make0 progress */
#define DEBUG_BIND     ( globs.debug[ 3 ] )   /* show when files bound */

#define DEBUG_EXECCMD  ( globs.debug[ 4 ] )   /* show execcmds()'s work */

#define DEBUG_COMPILE  ( globs.debug[ 5 ] )   /* show rule invocations */

#define DEBUG_HEADER   ( globs.debug[ 6 ] )   /* show result of header scan */
#define DEBUG_BINDSCAN ( globs.debug[ 6 ] )   /* show result of dir scan */
#define DEBUG_SEARCH   ( globs.debug[ 6 ] )   /* show binding attempts */

#define DEBUG_VARSET   ( globs.debug[ 7 ] )   /* show variable settings */
#define DEBUG_VARGET   ( globs.debug[ 8 ] )   /* show variable fetches */
#define DEBUG_VAREXP   ( globs.debug[ 8 ] )   /* show variable expansions */
#define DEBUG_IF       ( globs.debug[ 8 ] )   /* show 'if' calculations */
#define DEBUG_LISTS    ( globs.debug[ 9 ] )   /* show list manipulation */
#define DEBUG_SCAN     ( globs.debug[ 9 ] )   /* show scanner tokens */
#define DEBUG_MEM      ( globs.debug[ 9 ] )   /* show memory use */

#define DEBUG_PROFILE  ( globs.debug[ 10 ] )  /* dump rule execution times */
#define DEBUG_PARSE    ( globs.debug[ 11 ] )  /* debug parsing */
#define DEBUG_GRAPH    ( globs.debug[ 12 ] )  /* debug dependencies */
#define DEBUG_FATE     ( globs.debug[ 13 ] )  /* show fate changes in make0() */

/* Everyone gets the memory definitions. */
#include "mem.h"

/* They also get the profile functions. */
#include "debug.h"

#endif
