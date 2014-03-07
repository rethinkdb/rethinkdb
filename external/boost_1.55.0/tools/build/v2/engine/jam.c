/*
 * /+\
 * +\   Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 * \+/
 *
 * This file is part of jam.
 *
 * License is hereby granted to use this software and distribute it freely, as
 * long as this copyright notice is retained and modifications are clearly
 * marked.
 *
 * ALL WARRANTIES ARE HEREBY DISCLAIMED.
 */

/* This file is ALSO:
 * Copyright 2001-2004 David Abrahams.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * jam.c - make redux
 *
 * See Jam.html for usage information.
 *
 * These comments document the code.
 *
 * The top half of the code is structured such:
 *
 *                       jam
 *                      / | \
 *                 +---+  |  \
 *                /       |   \
 *         jamgram     option  \
 *        /  |   \              \
 *       /   |    \              \
 *      /    |     \             |
 *  scan     |     compile      make
 *   |       |    /  | \       / |  \
 *   |       |   /   |  \     /  |   \
 *   |       |  /    |   \   /   |    \
 * jambase parse     |   rules  search make1
 *                   |           |      |   \
 *                   |           |      |    \
 *                   |           |      |     \
 *               builtins    timestamp command execute
 *                               |
 *                               |
 *                               |
 *                             filesys
 *
 *
 * The support routines are called by all of the above, but themselves are
 * layered thus:
 *
 *                     variable|expand
 *                      /      |   |
 *                     /       |   |
 *                    /        |   |
 *                 lists       |   pathsys
 *                    \        |
 *                     \      hash
 *                      \      |
 *                       \     |
 *                        \    |
 *                         \   |
 *                          \  |
 *                         object
 *
 * Roughly, the modules are:
 *
 *  builtins.c - jam's built-in rules
 *  command.c - maintain lists of commands
 *  compile.c - compile parsed jam statements
 *  exec*.c - execute a shell script on a specific OS
 *  file*.c - scan directories and archives on a specific OS
 *  hash.c - simple in-memory hashing routines
 *  hdrmacro.c - handle header file parsing for filename macro definitions
 *  headers.c - handle #includes in source files
 *  jambase.c - compilable copy of Jambase
 *  jamgram.y - jam grammar
 *  lists.c - maintain lists of strings
 *  make.c - bring a target up to date, once rules are in place
 *  make1.c - execute command to bring targets up to date
 *  object.c - string manipulation routines
 *  option.c - command line option processing
 *  parse.c - make and destroy parse trees as driven by the parser
 *  path*.c - manipulate file names on a specific OS
 *  hash.c - simple in-memory hashing routines
 *  regexp.c - Henry Spencer's regexp
 *  rules.c - access to RULEs, TARGETs, and ACTIONs
 *  scan.c - the jam yacc scanner
 *  search.c - find a target along $(SEARCH) or $(LOCATE)
 *  timestamp.c - get the timestamp of a file or archive member
 *  variable.c - handle jam multi-element variables
 */


#include "jam.h"
#include "patchlevel.h"

#include "builtins.h"
#include "class.h"
#include "compile.h"
#include "constants.h"
#include "filesys.h"
#include "function.h"
#include "hcache.h"
#include "lists.h"
#include "make.h"
#include "object.h"
#include "option.h"
#include "output.h"
#include "parse.h"
#include "cwd.h"
#include "rules.h"
#include "scan.h"
#include "search.h"
#include "strings.h"
#include "timestamp.h"
#include "variable.h"

/* Macintosh is "special" */
#ifdef OS_MAC
# include <QuickDraw.h>
#endif

/* And UNIX for this. */
#ifdef unix
# include <sys/utsname.h>
# include <signal.h>
#endif

struct globs globs =
{
    0,          /* noexec */
    1,          /* jobs */
    0,          /* quitquick */
    0,          /* newestfirst */
    0,          /* pipes action stdout and stderr merged to action output */
#ifdef OS_MAC
    { 0, 0 },   /* debug - suppress tracing output */
#else
    { 0, 1 },   /* debug ... */
#endif
    0,          /* output commands, not run them */
    0,          /* action timeout */
    0           /* maximum buffer size zero is all output */
};

/* Symbols to be defined as true for use in Jambase. */
static char * othersyms[] = { OSMAJOR, OSMINOR, OSPLAT, JAMVERSYM, 0 };


/* Known for sure:
 *  mac needs arg_enviro
 *  OS2 needs extern environ
 */

#ifdef OS_MAC
# define use_environ arg_environ
# ifdef MPW
    QDGlobals qd;
# endif
#endif

/* on Win32-LCC */
#if defined( OS_NT ) && defined( __LCC__ )
# define use_environ _environ
#endif

#if defined( __MWERKS__)
# define use_environ _environ
    extern char * * _environ;
#endif

#ifndef use_environ
# define use_environ environ
# if !defined( __WATCOM__ ) && !defined( OS_OS2 ) && !defined( OS_NT )
    extern char **environ;
# endif
#endif

#if YYDEBUG != 0
    extern int yydebug;
#endif

#ifndef NDEBUG
static void run_unit_tests()
{
# if defined( USE_EXECNT )
    extern void execnt_unit_test();
    execnt_unit_test();
# endif
    string_unit_test();
}
#endif

int anyhow = 0;

#ifdef HAVE_PYTHON
    extern PyObject * bjam_call         ( PyObject * self, PyObject * args );
    extern PyObject * bjam_import_rule  ( PyObject * self, PyObject * args );
    extern PyObject * bjam_define_action( PyObject * self, PyObject * args );
    extern PyObject * bjam_variable     ( PyObject * self, PyObject * args );
    extern PyObject * bjam_backtrace    ( PyObject * self, PyObject * args );
    extern PyObject * bjam_caller       ( PyObject * self, PyObject * args );
#endif

void regex_done();

char const * saved_argv0;

int main( int argc, char * * argv, char * * arg_environ )
{
    int                     n;
    char                  * s;
    struct bjam_option      optv[ N_OPTS ];
    char            const * all = "all";
    int                     status;
    int                     arg_c = argc;
    char          *       * arg_v = argv;
    char            const * progname = argv[ 0 ];
    module_t              * environ_module;

    saved_argv0 = argv[ 0 ];

    BJAM_MEM_INIT();

#ifdef OS_MAC
    InitGraf( &qd.thePort );
#endif

    --argc;
    ++argv;

    if ( getoptions( argc, argv, "-:l:m:d:j:p:f:gs:t:ano:qv", optv ) < 0 )
    {
        printf( "\nusage: %s [ options ] targets...\n\n", progname );

        printf( "-a      Build all targets, even if they are current.\n" );
        printf( "-dx     Set the debug level to x (0-9).\n" );
        printf( "-fx     Read x instead of Jambase.\n" );
        /* printf( "-g      Build from newest sources first.\n" ); */
        printf( "-jx     Run up to x shell commands concurrently.\n" );
        printf( "-lx     Limit actions to x number of seconds after which they are stopped.\n" );
        printf( "-mx     Maximum target output saved (kb), default is to save all output.\n" );
        printf( "-n      Don't actually execute the updating actions.\n" );
        printf( "-ox     Write the updating actions to file x.\n" );
        printf( "-px     x=0, pipes action stdout and stderr merged into action output.\n" );
        printf( "-q      Quit quickly as soon as a target fails.\n" );
        printf( "-sx=y   Set variable x=y, overriding environment.\n" );
        printf( "-tx     Rebuild x, even if it is up-to-date.\n" );
        printf( "-v      Print the version of jam and exit.\n" );
        printf( "--x     Option is ignored.\n\n" );

        exit( EXITBAD );
    }

    /* Version info. */
    if ( ( s = getoptval( optv, 'v', 0 ) ) )
    {
        printf( "Boost.Jam  Version %s. %s.\n", VERSION, OSMINOR );
        printf( "   Copyright 1993-2002 Christopher Seiwald and Perforce "
            "Software, Inc.\n" );
        printf( "   Copyright 2001 David Turner.\n" );
        printf( "   Copyright 2001-2004 David Abrahams.\n" );
        printf( "   Copyright 2002-2008 Rene Rivera.\n" );
        printf( "   Copyright 2003-2008 Vladimir Prus.\n" );
        return EXITOK;
    }

    /* Pick up interesting options. */
    if ( ( s = getoptval( optv, 'n', 0 ) ) )
    {
        ++globs.noexec;
        globs.debug[ 2 ] = 1;
    }

    if ( ( s = getoptval( optv, 'p', 0 ) ) )
    {
        /* Undocumented -p3 (acts like both -p1 -p2) means separate pipe action
         * stdout and stderr.
         */
        globs.pipe_action = atoi( s );
        if ( globs.pipe_action < 0 || 3 < globs.pipe_action )
        {
            printf( "Invalid pipe descriptor '%d', valid values are -p[0..3]."
                "\n", globs.pipe_action );
            exit( EXITBAD );
        }
    }

    if ( ( s = getoptval( optv, 'q', 0 ) ) )
        globs.quitquick = 1;

    if ( ( s = getoptval( optv, 'a', 0 ) ) )
        anyhow++;

    if ( ( s = getoptval( optv, 'j', 0 ) ) )
    {
        globs.jobs = atoi( s );
        if ( globs.jobs < 1 || globs.jobs > MAXJOBS )
        {
            printf( "Invalid value for the '-j' option, valid values are 1 "
                "through %d.\n", MAXJOBS );
            exit( EXITBAD );
        }
    }

    if ( ( s = getoptval( optv, 'g', 0 ) ) )
        globs.newestfirst = 1;

    if ( ( s = getoptval( optv, 'l', 0 ) ) )
        globs.timeout = atoi( s );

    if ( ( s = getoptval( optv, 'm', 0 ) ) )
        globs.max_buf = atoi( s ) * 1024;  /* convert to kb */

    /* Turn on/off debugging */
    for ( n = 0; ( s = getoptval( optv, 'd', n ) ); ++n )
    {
        int i;

        /* First -d, turn off defaults. */
        if ( !n )
            for ( i = 0; i < DEBUG_MAX; ++i )
                globs.debug[i] = 0;

        i = atoi( s );

        if ( ( i < 0 ) || ( i >= DEBUG_MAX ) )
        {
            printf( "Invalid debug level '%s'.\n", s );
            continue;
        }

        /* n turns on levels 1-n. */
        /* +n turns on level n. */
        if ( *s == '+' )
            globs.debug[ i ] = 1;
        else while ( i )
            globs.debug[ i-- ] = 1;
    }

    constants_init();
    cwd_init();

    {
        PROFILE_ENTER( MAIN );

#ifdef HAVE_PYTHON
        {
            PROFILE_ENTER( MAIN_PYTHON );
            Py_Initialize();
            {
                static PyMethodDef BjamMethods[] = {
                    {"call", bjam_call, METH_VARARGS,
                     "Call the specified bjam rule."},
                    {"import_rule", bjam_import_rule, METH_VARARGS,
                     "Imports Python callable to bjam."},
                    {"define_action", bjam_define_action, METH_VARARGS,
                     "Defines a command line action."},
                    {"variable", bjam_variable, METH_VARARGS,
                     "Obtains a variable from bjam's global module."},
                    {"backtrace", bjam_backtrace, METH_VARARGS,
                     "Returns bjam backtrace from the last call into Python."},
                    {"caller", bjam_caller, METH_VARARGS,
                     "Returns the module from which the last call into Python is made."},
                    {NULL, NULL, 0, NULL}
                };

                Py_InitModule( "bjam", BjamMethods );
            }
            PROFILE_EXIT( MAIN_PYTHON );
        }
#endif

#ifndef NDEBUG
        run_unit_tests();
#endif
#if YYDEBUG != 0
        if ( DEBUG_PARSE )
            yydebug = 1;
#endif

        /* Set JAMDATE. */
        {
            timestamp current;
            timestamp_current( &current );
            var_set( root_module(), constant_JAMDATE, list_new( outf_time(
                &current ) ), VAR_SET );
        }

        /* Set JAM_VERSION. */
        var_set( root_module(), constant_JAM_VERSION,
                 list_push_back( list_push_back( list_new(
                   object_new( VERSION_MAJOR_SYM ) ),
                   object_new( VERSION_MINOR_SYM ) ),
                   object_new( VERSION_PATCH_SYM ) ),
                   VAR_SET );

        /* Set JAMUNAME. */
#ifdef unix
        {
            struct utsname u;

            if ( uname( &u ) >= 0 )
            {
                var_set( root_module(), constant_JAMUNAME,
                         list_push_back(
                             list_push_back(
                                 list_push_back(
                                     list_push_back(
                                         list_new(
                                            object_new( u.sysname ) ),
                                         object_new( u.nodename ) ),
                                     object_new( u.release ) ),
                                 object_new( u.version ) ),
                             object_new( u.machine ) ), VAR_SET );
            }
        }
#endif  /* unix */

        /* Set JAM_TIMESTAMP_RESOLUTION. */
        {
            timestamp fmt_resolution[ 1 ];
            file_supported_fmt_resolution( fmt_resolution );
            var_set( root_module(), constant_JAM_TIMESTAMP_RESOLUTION, list_new(
                object_new( timestamp_timestr( fmt_resolution ) ) ), VAR_SET );
        }

        /* Load up environment variables. */

        /* First into the global module, with splitting, for backward
         * compatibility.
         */
        var_defines( root_module(), use_environ, 1 );

        environ_module = bindmodule( constant_ENVIRON );
        /* Then into .ENVIRON, without splitting. */
        var_defines( environ_module, use_environ, 0 );

        /*
         * Jam defined variables OS & OSPLAT. We load them after environment, so
         * that setting OS in environment does not change Jam's notion of the
         * current platform.
         */
        var_defines( root_module(), othersyms, 1 );

        /* Load up variables set on command line. */
        for ( n = 0; ( s = getoptval( optv, 's', n ) ); ++n )
        {
            char * symv[ 2 ];
            symv[ 0 ] = s;
            symv[ 1 ] = 0;
            var_defines( root_module(), symv, 1 );
            var_defines( environ_module, symv, 0 );
        }

        /* Set the ARGV to reflect the complete list of arguments of invocation.
         */
        for ( n = 0; n < arg_c; ++n )
            var_set( root_module(), constant_ARGV, list_new( object_new(
                arg_v[ n ] ) ), VAR_APPEND );

        /* Initialize built-in rules. */
        load_builtins();

        /* Add the targets in the command line to the update list. */
        for ( n = 1; n < arg_c; ++n )
        {
            if ( arg_v[ n ][ 0 ] == '-' )
            {
                char * f = "-:l:d:j:f:gs:t:ano:qv";
                for ( ; *f; ++f ) if ( *f == arg_v[ n ][ 1 ] ) break;
                if ( ( f[ 1 ] == ':' ) && ( arg_v[ n ][ 2 ] == '\0' ) ) ++n;
            }
            else
            {
                OBJECT * const target = object_new( arg_v[ n ] );
                mark_target_for_updating( target );
                object_free( target );
            }
        }

        if ( list_empty( targets_to_update() ) )
            mark_target_for_updating( constant_all );

        /* Parse ruleset. */
        {
            FRAME frame[ 1 ];
            frame_init( frame );
            for ( n = 0; ( s = getoptval( optv, 'f', n ) ); ++n )
            {
                OBJECT * const filename = object_new( s );
                parse_file( filename, frame );
                object_free( filename );
            }

            if ( !n )
                parse_file( constant_plus, frame );
        }

        status = yyanyerrors();

        /* Manually touch -t targets. */
        for ( n = 0; ( s = getoptval( optv, 't', n ) ); ++n )
        {
            OBJECT * const target = object_new( s );
            touch_target( target );
            object_free( target );
        }

        /* If an output file is specified, set globs.cmdout to that. */
        if ( ( s = getoptval( optv, 'o', 0 ) ) )
        {
            if ( !( globs.cmdout = fopen( s, "w" ) ) )
            {
                printf( "Failed to write to '%s'\n", s );
                exit( EXITBAD );
            }
            ++globs.noexec;
        }

        /* The build system may set the PARALLELISM variable to override -j
         * options.
         */
        {
            LIST * const p = var_get( root_module(), constant_PARALLELISM );
            if ( !list_empty( p ) )
            {
                int const j = atoi( object_str( list_front( p ) ) );
                if ( j < 1 || j > MAXJOBS )
                    printf( "Invalid value of PARALLELISM: %s. Valid values "
                        "are 1 through %d.\n", object_str( list_front( p ) ),
                        MAXJOBS );
                else
                    globs.jobs = j;
            }
        }

        /* KEEP_GOING overrides -q option. */
        {
            LIST * const p = var_get( root_module(), constant_KEEP_GOING );
            if ( !list_empty( p ) )
                globs.quitquick = atoi( object_str( list_front( p ) ) ) ? 0 : 1;
        }

        /* Now make target. */
        {
            PROFILE_ENTER( MAIN_MAKE );
            LIST * const targets = targets_to_update();
            if ( !list_empty( targets ) )
                status |= make( targets, anyhow );
            else
                status = last_update_now_status;
            PROFILE_EXIT( MAIN_MAKE );
        }

        PROFILE_EXIT( MAIN );
    }

    if ( DEBUG_PROFILE )
        profile_dump();


#ifdef OPT_HEADER_CACHE_EXT
    hcache_done();
#endif

    clear_targets_to_update();

    /* Widely scattered cleanup. */
    property_set_done();
    file_done();
    rules_done();
    timestamp_done();
    search_done();
    class_done();
    modules_done();
    regex_done();
    cwd_done();
    path_done();
    function_done();
    list_done();
    constants_done();
    object_done();

    /* Close cmdout. */
    if ( globs.cmdout )
        fclose( globs.cmdout );

#ifdef HAVE_PYTHON
    Py_Finalize();
#endif

    BJAM_MEM_CLOSE();

    return status ? EXITBAD : EXITOK;
}


/*
 * executable_path()
 */

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
char * executable_path( char const * argv0 )
{
    char buf[ 1024 ];
    DWORD const ret = GetModuleFileName( NULL, buf, sizeof( buf ) );
    return ( !ret || ret == sizeof( buf ) ) ? NULL : strdup( buf );
}
#elif defined(__APPLE__)  /* Not tested */
# include <mach-o/dyld.h>
char *executable_path( char const * argv0 )
{
    char buf[ 1024 ];
    uint32_t size = sizeof( buf );
    return _NSGetExecutablePath( buf, &size ) ? NULL : strdup( buf );
}
#elif defined(sun) || defined(__sun)  /* Not tested */
# include <stdlib.h>
char * executable_path( char const * argv0 )
{
    return strdup( getexecname() );
}
#elif defined(__FreeBSD__)
# include <sys/sysctl.h>
char * executable_path( char const * argv0 )
{
    int mib[ 4 ] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    char buf[ 1024 ];
    size_t size = sizeof( buf );
    sysctl( mib, 4, buf, &size, NULL, 0 );
    return ( !size || size == sizeof( buf ) ) ? NULL : strndup( buf, size );
}
#elif defined(__linux__)
# include <unistd.h>
char * executable_path( char const * argv0 )
{
    char buf[ 1024 ];
    ssize_t const ret = readlink( "/proc/self/exe", buf, sizeof( buf ) );
    return ( !ret || ret == sizeof( buf ) ) ? NULL : strndup( buf, ret );
}
#else
char * executable_path( char const * argv0 )
{
    /* If argv0 is an absolute path, assume it is the right absolute path. */
    return argv0[ 0 ] == '/' ? strdup( argv0 ) : NULL;
}
#endif
