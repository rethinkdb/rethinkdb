/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

#include "jam.h"
#include "builtins.h"

#include "compile.h"
#include "constants.h"
#include "cwd.h"
#include "filesys.h"
#include "frames.h"
#include "hash.h"
#include "hdrmacro.h"
#include "lists.h"
#include "make.h"
#include "md5.h"
#include "native.h"
#include "object.h"
#include "parse.h"
#include "pathsys.h"
#include "rules.h"
#include "strings.h"
#include "subst.h"
#include "timestamp.h"
#include "variable.h"

#include <ctype.h>

#if defined(USE_EXECUNIX)
# include <sys/types.h>
# include <sys/wait.h>
#else
/*
 * NT does not have wait() and associated macros and uses the system() return
 * value instead. Status code group are documented at:
 * http://msdn.microsoft.com/en-gb/library/ff565436.aspx
 */
# define WIFEXITED(w)  (((w) & 0XFFFFFF00) == 0)
# define WEXITSTATUS(w)(w)
#endif

/*
 * builtins.c - builtin jam rules
 *
 * External routines:
 *  load_builtins()               - define builtin rules
 *  unknown_rule()                - reports an unknown rule occurrence to the
 *                                  user and exits
 *
 * Internal routines:
 *  append_if_exists()            - if file exists, append it to the list
 *  builtin_calc()                - CALC rule
 *  builtin_delete_module()       - DELETE_MODULE ( MODULE ? )
 *  builtin_depends()             - DEPENDS/INCLUDES rule
 *  builtin_echo()                - ECHO rule
 *  builtin_exit()                - EXIT rule
 *  builtin_export()              - EXPORT ( MODULE ? : RULES * )
 *  builtin_flags()               - NOCARE, NOTFILE, TEMPORARY rule
 *  builtin_glob()                - GLOB rule
 *  builtin_glob_recursive()      - ???
 *  builtin_hdrmacro()            - ???
 *  builtin_import()              - IMPORT rule
 *  builtin_match()               - MATCH rule, regexp matching
 *  builtin_rebuilds()            - REBUILDS rule
 *  builtin_rulenames()           - RULENAMES ( MODULE ? )
 *  builtin_split_by_characters() - splits the given string into tokens
 *  builtin_varnames()            - VARNAMES ( MODULE ? )
 *  get_source_line()             - get a frame's file and line number
 *                                  information
 */


/*
 * compile_builtin() - define builtin rules
 */

#define P0 (PARSE *)0
#define C0 (OBJECT *)0

#if defined( OS_NT ) || defined( OS_CYGWIN )
    LIST * builtin_system_registry      ( FRAME *, int );
    LIST * builtin_system_registry_names( FRAME *, int );
#endif

int glob( char const * s, char const * c );

void backtrace        ( FRAME * );
void backtrace_line   ( FRAME * );
void print_source_line( FRAME * );


RULE * bind_builtin( char const * name_, LIST * (* f)( FRAME *, int flags ),
    int flags, char const * * args )
{
    FUNCTION * func;
    RULE * result;
    OBJECT * name = object_new( name_ );

    func = function_builtin( f, flags, args );

    result = new_rule_body( root_module(), name, func, 1 );

    function_free( func );

    object_free( name );

    return result;
}


RULE * duplicate_rule( char const * name_, RULE * other )
{
    OBJECT * name = object_new( name_ );
    RULE * result = import_rule( other, root_module(), name );
    object_free( name );
    return result;
}


/*
 *  load_builtins() - define builtin rules
 */

void load_builtins()
{
    duplicate_rule( "Always",
      bind_builtin( "ALWAYS",
                    builtin_flags, T_FLAG_TOUCHED, 0 ) );

    duplicate_rule( "Depends",
      bind_builtin( "DEPENDS",
                    builtin_depends, 0, 0 ) );

    duplicate_rule( "echo",
    duplicate_rule( "Echo",
      bind_builtin( "ECHO",
                    builtin_echo, 0, 0 ) ) );

    {
        char const * args[] = { "message", "*", ":", "result-value", "?", 0 };
        duplicate_rule( "exit",
        duplicate_rule( "Exit",
          bind_builtin( "EXIT",
                        builtin_exit, 0, args ) ) );
    }

    {
        char const * args[] = { "directories", "*", ":", "patterns", "*", ":",
            "case-insensitive", "?", 0 };
        duplicate_rule( "Glob",
                        bind_builtin( "GLOB", builtin_glob, 0, args ) );
    }

    {
        char const * args[] = { "patterns", "*", 0 };
        bind_builtin( "GLOB-RECURSIVELY",
                      builtin_glob_recursive, 0, args );
    }

    duplicate_rule( "Includes",
      bind_builtin( "INCLUDES",
                    builtin_depends, 1, 0 ) );

    {
        char const * args[] = { "targets", "*", ":", "targets-to-rebuild", "*",
            0 };
        bind_builtin( "REBUILDS",
                      builtin_rebuilds, 0, args );
    }

    duplicate_rule( "Leaves",
      bind_builtin( "LEAVES",
                    builtin_flags, T_FLAG_LEAVES, 0 ) );

    duplicate_rule( "Match",
      bind_builtin( "MATCH",
                    builtin_match, 0, 0 ) );

    {
        char const * args[] = { "string", ":", "delimiters", 0 };
        bind_builtin( "SPLIT_BY_CHARACTERS",
                      builtin_split_by_characters, 0, args );
    }

    duplicate_rule( "NoCare",
      bind_builtin( "NOCARE",
                    builtin_flags, T_FLAG_NOCARE, 0 ) );

    duplicate_rule( "NOTIME",
    duplicate_rule( "NotFile",
      bind_builtin( "NOTFILE",
                    builtin_flags, T_FLAG_NOTFILE, 0 ) ) );

    duplicate_rule( "NoUpdate",
      bind_builtin( "NOUPDATE",
                    builtin_flags, T_FLAG_NOUPDATE, 0 ) );

    duplicate_rule( "Temporary",
      bind_builtin( "TEMPORARY",
                    builtin_flags, T_FLAG_TEMP, 0 ) );

      bind_builtin( "ISFILE",
                    builtin_flags, T_FLAG_ISFILE, 0 );

    duplicate_rule( "HdrMacro",
      bind_builtin( "HDRMACRO",
                    builtin_hdrmacro, 0, 0 ) );

    /* FAIL_EXPECTED is used to indicate that the result of a target build
     * action should be inverted (ok <=> fail) this can be useful when
     * performing test runs from Jamfiles.
     */
    bind_builtin( "FAIL_EXPECTED",
                  builtin_flags, T_FLAG_FAIL_EXPECTED, 0 );

    bind_builtin( "RMOLD",
                  builtin_flags, T_FLAG_RMOLD, 0 );

    {
        char const * args[] = { "targets", "*", 0 };
        bind_builtin( "UPDATE",
                      builtin_update, 0, args );
    }

    {
        char const * args[] = { "targets", "*",
                            ":", "log", "?",
                            ":", "ignore-minus-n", "?",
                            ":", "ignore-minus-q", "?", 0 };
        bind_builtin( "UPDATE_NOW",
                      builtin_update_now, 0, args );
    }

    {
        char const * args[] = { "string", "pattern", "replacements", "+", 0 };
        duplicate_rule( "subst",
          bind_builtin( "SUBST",
                        builtin_subst, 0, args ) );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "RULENAMES",
                       builtin_rulenames, 0, args );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "VARNAMES",
                       builtin_varnames, 0, args );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "DELETE_MODULE",
                       builtin_delete_module, 0, args );
    }

    {
        char const * args[] = { "source_module", "?",
                            ":", "source_rules", "*",
                            ":", "target_module", "?",
                            ":", "target_rules", "*",
                            ":", "localize", "?", 0 };
        bind_builtin( "IMPORT",
                      builtin_import, 0, args );
    }

    {
        char const * args[] = { "module", "?", ":", "rules", "*", 0 };
        bind_builtin( "EXPORT",
                      builtin_export, 0, args );
    }

    {
        char const * args[] = { "levels", "?", 0 };
        bind_builtin( "CALLER_MODULE",
                       builtin_caller_module, 0, args );
    }

    {
        char const * args[] = { "levels", "?", 0 };
        bind_builtin( "BACKTRACE",
                      builtin_backtrace, 0, args );
    }

    {
        char const * args[] = { 0 };
        bind_builtin( "PWD",
                      builtin_pwd, 0, args );
    }

    {
        char const * args[] = { "modules_to_import", "+",
                            ":", "target_module", "?", 0 };
        bind_builtin( "IMPORT_MODULE",
                      builtin_import_module, 0, args );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "IMPORTED_MODULES",
                      builtin_imported_modules, 0, args );
    }

    {
        char const * args[] = { "instance_module", ":", "class_module", 0 };
        bind_builtin( "INSTANCE",
                      builtin_instance, 0, args );
    }

    {
        char const * args[] = { "sequence", "*", 0 };
        bind_builtin( "SORT",
                      builtin_sort, 0, args );
    }

    {
        char const * args[] = { "path_parts", "*", 0 };
        bind_builtin( "NORMALIZE_PATH",
                      builtin_normalize_path, 0, args );
    }

    {
        char const * args[] = { "args", "*", 0 };
        bind_builtin( "CALC",
                      builtin_calc, 0, args );
    }

    {
        char const * args[] = { "module", ":", "rule", 0 };
        bind_builtin( "NATIVE_RULE",
                      builtin_native_rule, 0, args );
    }

    {
        char const * args[] = { "module", ":", "rule", ":", "version", 0 };
        bind_builtin( "HAS_NATIVE_RULE",
                      builtin_has_native_rule, 0, args );
    }

    {
        char const * args[] = { "module", "*", 0 };
        bind_builtin( "USER_MODULE",
                      builtin_user_module, 0, args );
    }

    {
        char const * args[] = { 0 };
        bind_builtin( "NEAREST_USER_LOCATION",
                      builtin_nearest_user_location, 0, args );
    }

    {
        char const * args[] = { "file", 0 };
        bind_builtin( "CHECK_IF_FILE",
                      builtin_check_if_file, 0, args );
    }

#ifdef HAVE_PYTHON
    {
        char const * args[] = { "python-module",
                            ":", "function",
                            ":", "jam-module",
                            ":", "rule-name", 0 };
        bind_builtin( "PYTHON_IMPORT_RULE",
                      builtin_python_import_rule, 0, args );
    }
#endif

# if defined( OS_NT ) || defined( OS_CYGWIN )
    {
        char const * args[] = { "key_path", ":", "data", "?", 0 };
        bind_builtin( "W32_GETREG",
                      builtin_system_registry, 0, args );
    }

    {
        char const * args[] = { "key_path", ":", "result-type", 0 };
        bind_builtin( "W32_GETREGNAMES",
                      builtin_system_registry_names, 0, args );
    }
# endif

    {
        char const * args[] = { "command", ":", "*", 0 };
        duplicate_rule( "SHELL",
          bind_builtin( "COMMAND",
                        builtin_shell, 0, args ) );
    }

    {
        char const * args[] = { "string", 0 };
        bind_builtin( "MD5",
                      builtin_md5, 0, args );
    }

    {
        char const * args[] = { "name", ":", "mode", 0 };
        bind_builtin( "FILE_OPEN",
                      builtin_file_open, 0, args );
    }

    {
        char const * args[] = { "string", ":", "width", 0 };
        bind_builtin( "PAD",
                      builtin_pad, 0, args );
    }

    {
        char const * args[] = { "targets", "*", 0 };
        bind_builtin( "PRECIOUS",
                      builtin_precious, 0, args );
    }

    {
        char const * args [] = { 0 };
        bind_builtin( "SELF_PATH", builtin_self_path, 0, args );
    }

    {
        char const * args [] = { "path", 0 };
        bind_builtin( "MAKEDIR", builtin_makedir, 0, args );
    }

    /* Initialize builtin modules. */
    init_set();
    init_path();
    init_regex();
    init_property_set();
    init_sequence();
    init_order();
}


/*
 * builtin_calc() - CALC rule
 *
 * Performs simple mathematical operations on two arguments.
 */

LIST * builtin_calc( FRAME * frame, int flags )
{
    LIST * arg = lol_get( frame->args, 0 );

    LIST * result = L0;
    long lhs_value;
    long rhs_value;
    long result_value;
    char buffer[ 16 ];
    char const * lhs;
    char const * op;
    char const * rhs;
    LISTITER iter = list_begin( arg );
    LISTITER const end = list_end( arg );

    if ( iter == end ) return L0;
    lhs = object_str( list_item( iter ) );

    iter = list_next( iter );
    if ( iter == end ) return L0;
    op = object_str( list_item( iter ) );

    iter = list_next( iter );
    if ( iter == end ) return L0;
    rhs = object_str( list_item( iter ) );

    lhs_value = atoi( lhs );
    rhs_value = atoi( rhs );

    if ( !strcmp( "+", op ) )
        result_value = lhs_value + rhs_value;
    else if ( !strcmp( "-", op ) )
        result_value = lhs_value - rhs_value;
    else
        return L0;

    sprintf( buffer, "%ld", result_value );
    result = list_push_back( result, object_new( buffer ) );
    return result;
}


/*
 * builtin_depends() - DEPENDS/INCLUDES rule
 *
 * The DEPENDS/INCLUDES builtin rule appends each of the listed sources on the
 * dependency/includes list of each of the listed targets. It binds both the
 * targets and sources as TARGETs.
 */

LIST * builtin_depends( FRAME * frame, int flags )
{
    LIST * const targets = lol_get( frame->args, 0 );
    LIST * const sources = lol_get( frame->args, 1 );

    LISTITER iter = list_begin( targets );
    LISTITER end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const t = bindtarget( list_item( iter ) );

        if ( flags )
            target_include_many( t, sources );
        else
            t->depends = targetlist( t->depends, sources );
    }

    /* Enter reverse links */
    iter = list_begin( sources );
    end = list_end( sources );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const s = bindtarget( list_item( iter ) );
        if ( flags )
        {
            LISTITER t_iter = list_begin( targets );
            LISTITER const t_end = list_end( targets );
            for ( ; t_iter != t_end; t_iter = list_next( t_iter ) )
                s->dependants = targetentry( s->dependants, bindtarget(
                    list_item( t_iter ) )->includes );
        }
        else
            s->dependants = targetlist( s->dependants, targets );
    }

    return L0;
}


/*
 * builtin_rebuilds() - REBUILDS rule
 *
 * Appends each of the rebuild-targets listed in its second argument to the
 * rebuilds list for each of the targets listed in its first argument.
 */

LIST * builtin_rebuilds( FRAME * frame, int flags )
{
    LIST * targets = lol_get( frame->args, 0 );
    LIST * rebuilds = lol_get( frame->args, 1 );
    LISTITER iter = list_begin( targets );
    LISTITER const end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const t = bindtarget( list_item( iter ) );
        t->rebuilds = targetlist( t->rebuilds, rebuilds );
    }
    return L0;
}


/*
 * builtin_echo() - ECHO rule
 *
 * Echoes the targets to the user. No other actions are taken.
 */

LIST * builtin_echo( FRAME * frame, int flags )
{
    list_print( lol_get( frame->args, 0 ) );
    printf( "\n" );
    fflush( stdout );
    return L0;
}


/*
 * builtin_exit() - EXIT rule
 *
 * Echoes the targets to the user and exits the program with a failure status.
 */

LIST * builtin_exit( FRAME * frame, int flags )
{
    LIST * const code = lol_get( frame->args, 1 );
    list_print( lol_get( frame->args, 0 ) );
    printf( "\n" );
    if ( !list_empty( code ) )
        exit( atoi( object_str( list_front( code ) ) ) );
    else
        exit( EXITBAD );  /* yeech */
    return L0;
}


/*
 * builtin_flags() - NOCARE, NOTFILE, TEMPORARY rule
 *
 * Marks the target with the appropriate flag, for use by make0(). It binds each
 * target as a TARGET.
 */

LIST * builtin_flags( FRAME * frame, int flags )
{
    LIST * const targets = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( targets );
    LISTITER const end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
        bindtarget( list_item( iter ) )->flags |= flags;
    return L0;
}


/*
 * builtin_glob() - GLOB rule
 */

struct globbing
{
    LIST * patterns;
    LIST * results;
    LIST * case_insensitive;
};


static void downcase_inplace( char * p )
{
    for ( ; *p; ++p )
        *p = tolower( *p );
}


static void builtin_glob_back( void * closure, OBJECT * file, int status,
    timestamp const * const time )
{
    PROFILE_ENTER( BUILTIN_GLOB_BACK );

    struct globbing * const globbing = (struct globbing *)closure;
    PATHNAME f;
    string buf[ 1 ];
    LISTITER iter;
    LISTITER end;

    /* Null out directory for matching. We wish we had file_dirscan() pass up a
     * PATHNAME.
     */
    path_parse( object_str( file ), &f );
    f.f_dir.len = 0;

    /* For globbing, we unconditionally ignore current and parent directory
     * items. Since these items always exist, there is no reason why caller of
     * GLOB would want to see them. We could also change file_dirscan(), but
     * then paths with embedded "." and ".." would not work anywhere.
    */
    if ( !strcmp( f.f_base.ptr, "." ) || !strcmp( f.f_base.ptr, ".." ) )
    {
        PROFILE_EXIT( BUILTIN_GLOB_BACK );
        return;
    }

    string_new( buf );
    path_build( &f, buf );

    if ( globbing->case_insensitive )
        downcase_inplace( buf->value );

    iter = list_begin( globbing->patterns );
    end = list_end( globbing->patterns );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        if ( !glob( object_str( list_item( iter ) ), buf->value ) )
        {
            globbing->results = list_push_back( globbing->results, object_copy(
                file ) );
            break;
        }
    }

    string_free( buf );

    PROFILE_EXIT( BUILTIN_GLOB_BACK );
}


static LIST * downcase_list( LIST * in )
{
    LIST * result = L0;
    LISTITER iter = list_begin( in );
    LISTITER const end = list_end( in );

    string s[ 1 ];
    string_new( s );

    for ( ; iter != end; iter = list_next( iter ) )
    {
        string_append( s, object_str( list_item( iter ) ) );
        downcase_inplace( s->value );
        result = list_push_back( result, object_new( s->value ) );
        string_truncate( s, 0 );
    }

    string_free( s );
    return result;
}


LIST * builtin_glob( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LIST * const r = lol_get( frame->args, 1 );

    LISTITER iter;
    LISTITER end;
    struct globbing globbing;

    globbing.results = L0;
    globbing.patterns = r;

    globbing.case_insensitive =
# if defined( OS_NT ) || defined( OS_CYGWIN )
       l;  /* Always case-insensitive if any files can be found. */
# else
       lol_get( frame->args, 2 );
# endif

    if ( globbing.case_insensitive )
        globbing.patterns = downcase_list( r );

    iter = list_begin( l );
    end = list_end( l );
    for ( ; iter != end; iter = list_next( iter ) )
        file_dirscan( list_item( iter ), builtin_glob_back, &globbing );

    if ( globbing.case_insensitive )
        list_free( globbing.patterns );

    return globbing.results;
}


static int has_wildcards( char const * const str )
{
    return str[ strcspn( str, "[]*?" ) ] ? 1 : 0;
}


/*
 * append_if_exists() - if file exists, append it to the list
 */

static LIST * append_if_exists( LIST * list, OBJECT * file )
{
    return file_query( file )
        ? list_push_back( list, object_copy( file ) )
        : list ;
}


LIST * glob1( OBJECT * dirname, OBJECT * pattern )
{
    LIST * const plist = list_new( object_copy( pattern ) );
    struct globbing globbing;

    globbing.results = L0;
    globbing.patterns = plist;

    globbing.case_insensitive
# if defined( OS_NT ) || defined( OS_CYGWIN )
       = plist;  /* always case-insensitive if any files can be found */
# else
       = L0;
# endif

    if ( globbing.case_insensitive )
        globbing.patterns = downcase_list( plist );

    file_dirscan( dirname, builtin_glob_back, &globbing );

    if ( globbing.case_insensitive )
        list_free( globbing.patterns );

    list_free( plist );

    return globbing.results;
}


LIST * glob_recursive( char const * pattern )
{
    LIST * result = L0;

    /* Check if there's metacharacters in pattern */
    if ( !has_wildcards( pattern ) )
    {
        /* No metacharacters. Check if the path exists. */
        OBJECT * const p = object_new( pattern );
        result = append_if_exists( result, p );
        object_free( p );
    }
    else
    {
        /* Have metacharacters in the pattern. Split into dir/name. */
        PATHNAME path[ 1 ];
        path_parse( pattern, path );

        if ( path->f_dir.ptr )
        {
            LIST * dirs = L0;
            string dirname[ 1 ];
            string basename[ 1 ];
            string_new( dirname );
            string_new( basename );

            string_append_range( dirname, path->f_dir.ptr,
                                path->f_dir.ptr + path->f_dir.len );

            path->f_grist.ptr = 0;
            path->f_grist.len = 0;
            path->f_dir.ptr = 0;
            path->f_dir.len = 0;
            path_build( path, basename );

            dirs =  has_wildcards( dirname->value )
                ? glob_recursive( dirname->value )
                : list_push_back( dirs, object_new( dirname->value ) );

            if ( has_wildcards( basename->value ) )
            {
                OBJECT * const b = object_new( basename->value );
                LISTITER iter = list_begin( dirs );
                LISTITER const end = list_end( dirs );
                for ( ; iter != end; iter = list_next( iter ) )
                    result = list_append( result, glob1( list_item( iter ), b )
                        );
                object_free( b );
            }
            else
            {
                LISTITER iter = list_begin( dirs );
                LISTITER const end = list_end( dirs );
                string file_string[ 1 ];
                string_new( file_string );

                /* No wildcard in basename. */
                for ( ; iter != end; iter = list_next( iter ) )
                {
                    OBJECT * p;
                    path->f_dir.ptr = object_str( list_item( iter ) );
                    path->f_dir.len = strlen( object_str( list_item( iter ) ) );
                    path_build( path, file_string );

                    p = object_new( file_string->value );

                    result = append_if_exists( result, p );

                    object_free( p );

                    string_truncate( file_string, 0 );
                }

                string_free( file_string );
            }

            string_free( dirname );
            string_free( basename );

            list_free( dirs );
        }
        else
        {
            /* No directory, just a pattern. */
            OBJECT * const p = object_new( pattern );
            result = list_append( result, glob1( constant_dot, p ) );
            object_free( p );
        }
    }

    return result;
}


/*
 * builtin_glob_recursive() - ???
 */

LIST * builtin_glob_recursive( FRAME * frame, int flags )
{
    LIST * result = L0;
    LIST * const l = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( l );
    LISTITER const end = list_end( l );
    for ( ; iter != end; iter = list_next( iter ) )
        result = list_append( result, glob_recursive( object_str( list_item(
            iter ) ) ) );
    return result;
}


/*
 * builtin_match() - MATCH rule, regexp matching
 */

LIST * builtin_match( FRAME * frame, int flags )
{
    LIST * l;
    LIST * r;
    LIST * result = L0;
    LISTITER l_iter;
    LISTITER l_end;
    LISTITER r_iter;
    LISTITER r_end;

    string buf[ 1 ];
    string_new( buf );

    /* For each pattern */

    l = lol_get( frame->args, 0 );
    l_iter = list_begin( l );
    l_end = list_end( l );
    for ( ; l_iter != l_end; l_iter = list_next( l_iter ) )
    {
        /* Result is cached and intentionally never freed. */
        regexp * re = regex_compile( list_item( l_iter ) );

        /* For each string to match against. */
        r = lol_get( frame->args, 1 );
        r_iter = list_begin( r );
        r_end = list_end( r );
        for ( ; r_iter != r_end; r_iter = list_next( r_iter ) )
        {
            if ( regexec( re, object_str( list_item( r_iter ) ) ) )
            {
                int i;
                int top;

                /* Find highest parameter */

                for ( top = NSUBEXP; top-- > 1; )
                    if ( re->startp[ top ] )
                        break;

                /* And add all parameters up to highest onto list. */
                /* Must have parameters to have results! */
                for ( i = 1; i <= top; ++i )
                {
                    string_append_range( buf, re->startp[ i ], re->endp[ i ] );
                    result = list_push_back( result, object_new( buf->value ) );
                    string_truncate( buf, 0 );
                }
            }
        }
    }

    string_free( buf );
    return result;
}


/*
 * builtin_split_by_characters() - splits the given string into tokens
 */

LIST * builtin_split_by_characters( FRAME * frame, int flags )
{
    LIST * l1 = lol_get( frame->args, 0 );
    LIST * l2 = lol_get( frame->args, 1 );

    LIST * result = L0;

    string buf[ 1 ];

    char const * delimiters = object_str( list_front( l2 ) );
    char * t;

    string_copy( buf, object_str( list_front( l1 ) ) );

    t = strtok( buf->value, delimiters );
    while ( t )
    {
        result = list_push_back( result, object_new( t ) );
        t = strtok( NULL, delimiters );
    }

    string_free( buf );

    return result;
}


/*
 * builtin_hdrmacro() - ???
 */

LIST * builtin_hdrmacro( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( l );
    LISTITER const end = list_end( l );

    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const t = bindtarget( list_item( iter ) );

        /* Scan file for header filename macro definitions. */
        if ( DEBUG_HEADER )
            printf( "scanning '%s' for header file macro definitions\n",
                object_str( list_item( iter ) ) );

        macro_headers( t );
    }

    return L0;
}


/*
 * builtin_rulenames() - RULENAMES ( MODULE ? )
 *
 * Returns a list of the non-local rule names in the given MODULE. If MODULE is
 * not supplied, returns the list of rule names in the global module.
 */

static void add_rule_name( void * r_, void * result_ )
{
    RULE * const r = (RULE *)r_;
    LIST * * const result = (LIST * *)result_;
    if ( r->exported )
        *result = list_push_back( *result, object_copy( r->name ) );
}


LIST * builtin_rulenames( FRAME * frame, int flags )
{
    LIST * arg0 = lol_get( frame->args, 0 );
    LIST * result = L0;
    module_t * const source_module = bindmodule( list_empty( arg0 )
        ? 0
        : list_front( arg0 ) );

    if ( source_module->rules )
        hashenumerate( source_module->rules, add_rule_name, &result );
    return result;
}


/*
 * builtin_varnames() - VARNAMES ( MODULE ? )
 *
 * Returns a list of the variable names in the given MODULE. If MODULE is not
 * supplied, returns the list of variable names in the global module.
 */

/* helper function for builtin_varnames(), below. Used with hashenumerate, will
 * prepend the key of each element to the list
 */
static void add_hash_key( void * np, void * result_ )
{
    LIST * * result = (LIST * *)result_;
    *result = list_push_back( *result, object_copy( *(OBJECT * *)np ) );
}


LIST * builtin_varnames( FRAME * frame, int flags )
{
    LIST * arg0 = lol_get( frame->args, 0 );
    LIST * result = L0;
    module_t * source_module = bindmodule( list_empty( arg0 )
        ? 0
        : list_front( arg0 ) );

    struct hash * const vars = source_module->variables;
    if ( vars )
        hashenumerate( vars, add_hash_key, &result );
    return result;
}


/*
 * builtin_delete_module() - DELETE_MODULE ( MODULE ? )
 *
 * Clears all rules and variables from the given module.
 */

LIST * builtin_delete_module( FRAME * frame, int flags )
{
    LIST * const arg0 = lol_get( frame->args, 0 );
    module_t * const source_module = bindmodule( list_empty( arg0 ) ? 0 :
        list_front( arg0 ) );
    delete_module( source_module );
    return L0;
}


/*
 * unknown_rule() - reports an unknown rule occurrence to the user and exits
 */

void unknown_rule( FRAME * frame, char const * key, module_t * module,
    OBJECT * rule_name )
{
    backtrace_line( frame->prev );
    if ( key )
        printf("%s error", key);
    else
        printf("ERROR");
    printf( ": rule \"%s\" unknown in ", object_str( rule_name ) );
    if ( module->name )
        printf( "module \"%s\".\n", object_str( module->name ) );
    else
        printf( "root module.\n" );
    backtrace( frame->prev );
    exit( 1 );
}


/*
 * builtin_import() - IMPORT rule
 *
 * IMPORT
 * (
 *     SOURCE_MODULE ? :
 *     SOURCE_RULES  * :
 *     TARGET_MODULE ? :
 *     TARGET_RULES  * :
 *     LOCALIZE      ?
 * )
 *
 * Imports rules from the SOURCE_MODULE into the TARGET_MODULE as local rules.
 * If either SOURCE_MODULE or TARGET_MODULE is not supplied, it refers to the
 * global module. SOURCE_RULES specifies which rules from the SOURCE_MODULE to
 * import; TARGET_RULES specifies the names to give those rules in
 * TARGET_MODULE. If SOURCE_RULES contains a name that does not correspond to
 * a rule in SOURCE_MODULE, or if it contains a different number of items than
 * TARGET_RULES, an error is issued. If LOCALIZE is specified, the rules will be
 * executed in TARGET_MODULE, with corresponding access to its module local
 * variables.
 */

LIST * builtin_import( FRAME * frame, int flags )
{
    LIST * source_module_list = lol_get( frame->args, 0 );
    LIST * source_rules       = lol_get( frame->args, 1 );
    LIST * target_module_list = lol_get( frame->args, 2 );
    LIST * target_rules       = lol_get( frame->args, 3 );
    LIST * localize           = lol_get( frame->args, 4 );

    module_t * target_module = bindmodule( list_empty( target_module_list )
        ? 0
        : list_front( target_module_list ) );
    module_t * source_module = bindmodule( list_empty( source_module_list )
        ? 0
        : list_front( source_module_list ) );

    LISTITER source_iter = list_begin( source_rules );
    LISTITER const source_end = list_end( source_rules );
    LISTITER target_iter = list_begin( target_rules );
    LISTITER const target_end = list_end( target_rules );

    for ( ;
          source_iter != source_end && target_iter != target_end;
          source_iter = list_next( source_iter ),
          target_iter = list_next( target_iter ) )
    {
        RULE * r;
        RULE * imported;

        if ( !source_module->rules || !(r = (RULE *)hash_find(
            source_module->rules, list_item( source_iter ) ) ) )
            unknown_rule( frame, "IMPORT", source_module, list_item( source_iter
                ) );

        imported = import_rule( r, target_module, list_item( target_iter ) );
        if ( !list_empty( localize ) )
            rule_localize( imported, target_module );
        /* This rule is really part of some other module. Just refer to it here,
         * but do not let it out.
         */
        imported->exported = 0;
    }

    if ( source_iter != source_end || target_iter != target_end )
    {
        backtrace_line( frame->prev );
        printf( "import error: length of source and target rule name lists "
            "don't match!\n" );
        printf( "    source: " );
        list_print( source_rules );
        printf( "\n    target: " );
        list_print( target_rules );
        printf( "\n" );
        backtrace( frame->prev );
        exit( 1 );
    }

    return L0;
}


/*
 * builtin_export() - EXPORT ( MODULE ? : RULES * )
 *
 * The EXPORT rule marks RULES from the SOURCE_MODULE as non-local (and thus
 * exportable). If an element of RULES does not name a rule in MODULE, an error
 * is issued.
 */

LIST * builtin_export( FRAME * frame, int flags )
{
    LIST * const module_list = lol_get( frame->args, 0 );
    LIST * const rules = lol_get( frame->args, 1 );
    module_t * const m = bindmodule( list_empty( module_list ) ? 0 : list_front(
        module_list ) );

    LISTITER iter = list_begin( rules );
    LISTITER const end = list_end( rules );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        RULE * r;
        if ( !m->rules || !( r = (RULE *)hash_find( m->rules, list_item( iter )
            ) ) )
            unknown_rule( frame, "EXPORT", m, list_item( iter ) );
        r->exported = 1;
    }
    return L0;
}


/*
 * get_source_line() - get a frame's file and line number information
 *
 * This is the execution traceback information to be indicated for in debug
 * output or an error backtrace.
 */

static void get_source_line( FRAME * frame, char const * * file, int * line )
{
    if ( frame->file )
    {
        char const * f = object_str( frame->file );
        int l = frame->line;
        if ( !strcmp( f, "+" ) )
        {
            f = "jambase.c";
            l += 3;
        }
        *file = f;
        *line = l;
    }
    else
    {
        *file = "(builtin)";
        *line = -1;
    }
}


void print_source_line( FRAME * frame )
{
    char const * file;
    int line;
    get_source_line( frame, &file, &line );
    if ( line < 0 )
        printf( "(builtin):" );
    else
        printf( "%s:%d:", file, line );
}


/*
 * backtrace_line() - print a single line of error backtrace for the given
 * frame.
 */

void backtrace_line( FRAME * frame )
{
    if ( frame == 0 )
    {
        printf( "(no frame):" );
    }
    else
    {
        print_source_line( frame );
        printf( " in %s\n", frame->rulename );
    }
}


/*
 * backtrace() - Print the entire backtrace from the given frame to the Jambase
 * which invoked it.
 */

void backtrace( FRAME * frame )
{
    if ( !frame ) return;
    while ( ( frame = frame->prev ) )
        backtrace_line( frame );
}


/*
 * builtin_backtrace() - A Jam version of the backtrace function, taking no
 * arguments and returning a list of quadruples: FILENAME LINE MODULE. RULENAME
 * describing each frame. Note that the module-name is always followed by a
 * period.
 */

LIST * builtin_backtrace( FRAME * frame, int flags )
{
    LIST * const levels_arg = lol_get( frame->args, 0 );
    int levels = list_empty( levels_arg )
        ? (int)( (unsigned int)(-1) >> 1 )
        : atoi( object_str( list_front( levels_arg ) ) );

    LIST * result = L0;
    for ( ; ( frame = frame->prev ) && levels; --levels )
    {
        char const * file;
        int line;
        char buf[ 32 ];
        string module_name[ 1 ];
        get_source_line( frame, &file, &line );
        sprintf( buf, "%d", line );
        string_new( module_name );
        if ( frame->module->name )
        {
            string_append( module_name, object_str( frame->module->name ) );
            string_append( module_name, "." );
        }
        result = list_push_back( result, object_new( file ) );
        result = list_push_back( result, object_new( buf ) );
        result = list_push_back( result, object_new( module_name->value ) );
        result = list_push_back( result, object_new( frame->rulename ) );
        string_free( module_name );
    }
    return result;
}


/*
 * builtin_caller_module() - CALLER_MODULE ( levels ? )
 *
 * If levels is not supplied, returns the name of the module of the rule which
 * called the one calling this one. If levels is supplied, it is interpreted as
 * an integer specifying a number of additional levels of call stack to traverse
 * in order to locate the module in question. If no such module exists, returns
 * the empty list. Also returns the empty list when the module in question is
 * the global module. This rule is needed for implementing module import
 * behavior.
 */

LIST * builtin_caller_module( FRAME * frame, int flags )
{
    LIST * const levels_arg = lol_get( frame->args, 0 );
    int const levels = list_empty( levels_arg )
        ? 0
        : atoi( object_str( list_front( levels_arg ) ) );

    int i;
    for ( i = 0; ( i < levels + 2 ) && frame->prev; ++i )
        frame = frame->prev;

    return frame->module == root_module()
        ? L0
        : list_new( object_copy( frame->module->name ) );
}


/*
 * Return the current working directory.
 *
 * Usage: pwd = [ PWD ] ;
 */

LIST * builtin_pwd( FRAME * frame, int flags )
{
    return list_new( object_copy( cwd() ) );
}


/*
 * Adds targets to the list of target that jam will attempt to update.
 */

LIST * builtin_update( FRAME * frame, int flags )
{
    LIST * result = list_copy( targets_to_update() );
    LIST * arg1 = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( arg1 ), end = list_end( arg1 );
    clear_targets_to_update();
    for ( ; iter != end; iter = list_next( iter ) )
        mark_target_for_updating( object_copy( list_item( iter ) ) );
    return result;
}

extern int anyhow;
int last_update_now_status;

/* Takes a list of target names and immediately updates them.
 *
 * Parameters:
 *  1. Target list.
 *  2. Optional file descriptor (converted to a string) for a log file where all
 *     the related build output should be redirected.
 *  3. If specified, makes the build temporarily disable the -n option, i.e.
 *     forces all needed out-of-date targets to be rebuilt.
 *  4. If specified, makes the build temporarily disable the -q option, i.e.
 *     forces the build to continue even if one of the targets fails to build.
 */
LIST * builtin_update_now( FRAME * frame, int flags )
{
    LIST * targets = lol_get( frame->args, 0 );
    LIST * log = lol_get( frame->args, 1 );
    LIST * force = lol_get( frame->args, 2 );
    LIST * continue_ = lol_get( frame->args, 3 );
    int status;
    int original_stdout = 0;
    int original_stderr = 0;
    int original_noexec = 0;
    int original_quitquick = 0;

    if ( !list_empty( log ) )
    {
        /* Temporarily redirect stdout and stderr to the given log file. */
        int const fd = atoi( object_str( list_front( log ) ) );
        original_stdout = dup( 0 );
        original_stderr = dup( 1 );
        dup2( fd, 0 );
        dup2( fd, 1 );
    }

    if ( !list_empty( force ) )
    {
        original_noexec = globs.noexec;
        globs.noexec = 0;
    }

    if ( !list_empty( continue_ ) )
    {
        original_quitquick = globs.quitquick;
        globs.quitquick = 0;
    }

    status = make( targets, anyhow );

    if ( !list_empty( force ) )
    {
        globs.noexec = original_noexec;
    }

    if ( !list_empty( continue_ ) )
    {
        globs.quitquick = original_quitquick;
    }

    if ( !list_empty( log ) )
    {
        /* Flush whatever stdio might have buffered, while descriptions 0 and 1
         * still refer to the log file.
         */
        fflush( stdout );
        fflush( stderr );
        dup2( original_stdout, 0 );
        dup2( original_stderr, 1 );
        close( original_stdout );
        close( original_stderr );
    }

    last_update_now_status = status;

    return status ? L0 : list_new( object_copy( constant_ok ) );
}


LIST * builtin_import_module( FRAME * frame, int flags )
{
    LIST * const arg1 = lol_get( frame->args, 0 );
    LIST * const arg2 = lol_get( frame->args, 1 );
    module_t * const m = list_empty( arg2 )
        ? root_module()
        : bindmodule( list_front( arg2 ) );
    import_module( arg1, m );
    return L0;
}


LIST * builtin_imported_modules( FRAME * frame, int flags )
{
    LIST * const arg0 = lol_get( frame->args, 0 );
    OBJECT * const module = list_empty( arg0 ) ? 0 : list_front( arg0 );
    return imported_modules( bindmodule( module ) );
}


LIST * builtin_instance( FRAME * frame, int flags )
{
    LIST * arg1 = lol_get( frame->args, 0 );
    LIST * arg2 = lol_get( frame->args, 1 );
    module_t * const instance     = bindmodule( list_front( arg1 ) );
    module_t * const class_module = bindmodule( list_front( arg2 ) );
    instance->class_module = class_module;
    module_set_fixed_variables( instance, class_module->num_fixed_variables );
    return L0;
}


LIST * builtin_sort( FRAME * frame, int flags )
{
    return list_sort( lol_get( frame->args, 0 ) );
}


LIST * builtin_normalize_path( FRAME * frame, int flags )
{
    LIST * arg = lol_get( frame->args, 0 );

    /* First, we iterate over all '/'-separated elements, starting from the end
     * of string. If we see a '..', we remove a preceeding path element. If we
     * see '.', we remove it. Removal is done by overwriting data using '\1'
     * characters. After the whole string has been processed, we do a second
     * pass, removing any entered '\1' characters.
     */

    string   in[ 1 ];
    string   out[ 1 ];
    /* Last character of the part of string still to be processed. */
    char   * end;
    /* Working pointer. */
    char   * current;
    /* Number of '..' elements seen and not processed yet. */
    int      dotdots = 0;
    int      rooted  = 0;
    OBJECT * result  = 0;
    LISTITER arg_iter = list_begin( arg );
    LISTITER arg_end = list_end( arg );

    /* Make a copy of input: we should not change it. Prepend a '/' before it as
     * a guard for the algorithm later on and remember whether it was originally
     * rooted or not.
     */
    string_new( in );
    string_push_back( in, '/' );
    for ( ; arg_iter != arg_end; arg_iter = list_next( arg_iter ) )
    {
        if ( object_str( list_item( arg_iter ) )[ 0 ] != '\0' )
        {
            if ( in->size == 1 )
                rooted = ( object_str( list_item( arg_iter ) )[ 0 ] == '/'  ) ||
                         ( object_str( list_item( arg_iter ) )[ 0 ] == '\\' );
            else
                string_append( in, "/" );
            string_append( in, object_str( list_item( arg_iter ) ) );
        }
    }

    /* Convert \ into /. On Windows, paths using / and \ are equivalent, and we
     * want this function to obtain a canonic representation.
     */
    for ( current = in->value, end = in->value + in->size;
        current < end; ++current )
        if ( *current == '\\' )
            *current = '/';

    /* Now we remove any extra path elements by overwriting them with '\1'
     * characters and cound how many more unused '..' path elements there are
     * remaining. Note that each remaining path element with always starts with
     * a '/' character.
     */
    for ( end = in->value + in->size - 1; end >= in->value; )
    {
        /* Set 'current' to the next occurence of '/', which always exists. */
        for ( current = end; *current != '/'; --current );

        if ( current == end )
        {
            /* Found a trailing or duplicate '/'. Remove it. */
            *current = '\1';
        }
        else if ( ( end - current == 1 ) && ( *( current + 1 ) == '.' ) )
        {
            /* Found '/.'. Remove them all. */
            *current = '\1';
            *(current + 1) = '\1';
        }
        else if ( ( end - current == 2 ) && ( *( current + 1 ) == '.' ) &&
            ( *( current + 2 ) == '.' ) )
        {
            /* Found '/..'. Remove them all. */
            *current = '\1';
            *(current + 1) = '\1';
            *(current + 2) = '\1';
            ++dotdots;
        }
        else if ( dotdots )
        {
            memset( current, '\1', end - current + 1 );
            --dotdots;
        }
        end = current - 1;
    }

    string_new( out );

    /* Now we know that we need to add exactly dotdots '..' path elements to the
     * front and that our string is either empty or has a '/' as its first
     * significant character. If we have any dotdots remaining then the passed
     * path must not have been rooted or else it is invalid we return an empty
     * list.
     */
    if ( dotdots )
    {
        if ( rooted )
        {
            string_free( out );
            string_free( in );
            return L0;
        }
        do
            string_append( out, "/.." );
        while ( --dotdots );
    }

    /* Now we actually remove all the path characters marked for removal. */
    for ( current = in->value; *current; ++current )
        if ( *current != '\1' )
            string_push_back( out, *current );

    /* Here we know that our string contains no '\1' characters and is either
     * empty or has a '/' as its initial character. If the original path was not
     * rooted and we have a non-empty path we need to drop the initial '/'. If
     * the original path was rooted and we have an empty path we need to add
     * back the '/'.
     */
    result = object_new( out->size
        ? out->value + !rooted
        : ( rooted ? "/" : "." ) );

    string_free( out );
    string_free( in );

    return list_new( result );
}


LIST * builtin_native_rule( FRAME * frame, int flags )
{
    LIST * module_name = lol_get( frame->args, 0 );
    LIST * rule_name = lol_get( frame->args, 1 );

    module_t * module = bindmodule( list_front( module_name ) );

    native_rule_t * np;
    if ( module->native_rules && (np = (native_rule_t *)hash_find(
        module->native_rules, list_front( rule_name ) ) ) )
    {
        new_rule_body( module, np->name, np->procedure, 1 );
    }
    else
    {
        backtrace_line( frame->prev );
        printf( "error: no native rule \"%s\" defined in module \"%s.\"\n",
            object_str( list_front( rule_name ) ), object_str( module->name ) );
        backtrace( frame->prev );
        exit( 1 );
    }
    return L0;
}


LIST * builtin_has_native_rule( FRAME * frame, int flags )
{
    LIST * module_name = lol_get( frame->args, 0 );
    LIST * rule_name   = lol_get( frame->args, 1 );
    LIST * version     = lol_get( frame->args, 2 );

    module_t * module = bindmodule( list_front( module_name ) );

    native_rule_t * np;
    if ( module->native_rules && (np = (native_rule_t *)hash_find(
        module->native_rules, list_front( rule_name ) ) ) )
    {
        int expected_version = atoi( object_str( list_front( version ) ) );
        if ( np->version == expected_version )
            return list_new( object_copy( constant_true ) );
    }
    return L0;
}


LIST * builtin_user_module( FRAME * frame, int flags )
{
    LIST * const module_name = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( module_name );
    LISTITER const end = list_end( module_name );
    for ( ; iter != end; iter = list_next( iter ) )
        bindmodule( list_item( iter ) )->user_module = 1;
    return L0;
}


LIST * builtin_nearest_user_location( FRAME * frame, int flags )
{
    FRAME * const nearest_user_frame = frame->module->user_module
        ? frame
        : frame->prev_user;
    if ( !nearest_user_frame )
        return L0;

    {
        LIST * result = L0;
        char const * file;
        int line;
        char buf[ 32 ];

        get_source_line( nearest_user_frame, &file, &line );
        sprintf( buf, "%d", line );
        result = list_push_back( result, object_new( file ) );
        result = list_push_back( result, object_new( buf ) );
        return result;
    }
}


LIST * builtin_check_if_file( FRAME * frame, int flags )
{
    LIST * const name = lol_get( frame->args, 0 );
    return file_is_file( list_front( name ) ) == 1
        ? list_new( object_copy( constant_true ) )
        : L0;
}


LIST * builtin_md5( FRAME * frame, int flags )
{
    LIST * l = lol_get( frame->args, 0 );
    char const * s = object_str( list_front( l ) );

    md5_state_t state;
    md5_byte_t digest[ 16 ];
    char hex_output[ 16 * 2 + 1 ];

    int di;

    md5_init( &state );
    md5_append( &state, (md5_byte_t const *)s, strlen( s ) );
    md5_finish( &state, digest );

    for ( di = 0; di < 16; ++di )
        sprintf( hex_output + di * 2, "%02x", digest[ di ] );

    return list_new( object_new( hex_output ) );
}


LIST * builtin_file_open( FRAME * frame, int flags )
{
    char const * name = object_str( list_front( lol_get( frame->args, 0 ) ) );
    char const * mode = object_str( list_front( lol_get( frame->args, 1 ) ) );
    int fd;
    char buffer[ sizeof( "4294967295" ) ];

    if ( strcmp(mode, "w") == 0 )
        fd = open( name, O_WRONLY|O_CREAT|O_TRUNC, 0666 );
    else
        fd = open( name, O_RDONLY );

    if ( fd != -1 )
    {
        sprintf( buffer, "%d", fd );
        return list_new( object_new( buffer ) );
    }
    return L0;
}


LIST * builtin_pad( FRAME * frame, int flags )
{
    OBJECT * string = list_front( lol_get( frame->args, 0 ) );
    char const * width_s = object_str( list_front( lol_get( frame->args, 1 ) ) );

    int current = strlen( object_str( string ) );
    int desired = atoi( width_s );
    if ( current >= desired )
        return list_new( object_copy( string ) );
    else
    {
        char * buffer = BJAM_MALLOC( desired + 1 );
        int i;
        LIST * result;

        strcpy( buffer, object_str( string ) );
        for ( i = current; i < desired; ++i )
            buffer[ i ] = ' ';
        buffer[ desired ] = '\0';
        result = list_new( object_new( buffer ) );
        BJAM_FREE( buffer );
        return result;
    }
}


LIST * builtin_precious( FRAME * frame, int flags )
{
    LIST * targets = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( targets );
    LISTITER const end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
        bindtarget( list_item( iter ) )->flags |= T_FLAG_PRECIOUS;
    return L0;
}


LIST * builtin_self_path( FRAME * frame, int flags )
{
    extern char const * saved_argv0;
    char * p = executable_path( saved_argv0 );
    if ( p )
    {
        LIST * const result = list_new( object_new( p ) );
        free( p );
        return result;
    }
    return L0;
}


LIST * builtin_makedir( FRAME * frame, int flags )
{
    LIST * const path = lol_get( frame->args, 0 );
    return file_mkdir( object_str( list_front( path ) ) )
        ? L0
        : list_new( object_copy( list_front( path ) ) );
}


#ifdef HAVE_PYTHON

LIST * builtin_python_import_rule( FRAME * frame, int flags )
{
    static int first_time = 1;
    char const * python_module   = object_str( list_front( lol_get( frame->args,
        0 ) ) );
    char const * python_function = object_str( list_front( lol_get( frame->args,
        1 ) ) );
    OBJECT     * jam_module      = list_front( lol_get( frame->args, 2 ) );
    OBJECT     * jam_rule        = list_front( lol_get( frame->args, 3 ) );

    PyObject * pName;
    PyObject * pModule;
    PyObject * pDict;
    PyObject * pFunc;

    if ( first_time )
    {
        /* At the first invocation, we add the value of the global
         * EXTRA_PYTHONPATH to the sys.path Python variable.
         */
        LIST * extra = 0;
        module_t * outer_module = frame->module;
        LISTITER iter, end;

        first_time = 0;

        extra = var_get( root_module(), constant_extra_pythonpath );

        iter = list_begin( extra ), end = list_end( extra );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            string buf[ 1 ];
            string_new( buf );
            string_append( buf, "import sys\nsys.path.append(\"" );
            string_append( buf, object_str( list_item( iter ) ) );
            string_append( buf, "\")\n" );
            PyRun_SimpleString( buf->value );
            string_free( buf );
        }
    }

    pName   = PyString_FromString( python_module );
    pModule = PyImport_Import( pName );
    Py_DECREF( pName );

    if ( pModule != NULL )
    {
        pDict = PyModule_GetDict( pModule );
        pFunc = PyDict_GetItemString( pDict, python_function );

        if ( pFunc && PyCallable_Check( pFunc ) )
        {
            module_t * m = bindmodule( jam_module );
            new_rule_body( m, jam_rule, function_python( pFunc, 0 ), 0 );
        }
        else
        {
            if ( PyErr_Occurred() )
                PyErr_Print();
            fprintf( stderr, "Cannot find function \"%s\"\n", python_function );
        }
        Py_DECREF( pModule );
    }
    else
    {
        PyErr_Print();
        fprintf( stderr, "Failed to load \"%s\"\n", python_module );
    }
    return L0;

}

#endif  /* #ifdef HAVE_PYTHON */


void lol_build( LOL * lol, char const * * elements )
{
    LIST * l = L0;
    lol_init( lol );

    while ( elements && *elements )
    {
        if ( !strcmp( *elements, ":" ) )
        {
            lol_add( lol, l );
            l = L0;
        }
        else
        {
            l = list_push_back( l, object_new( *elements ) );
        }
        ++elements;
    }

    if ( l != L0 )
        lol_add( lol, l );
}


#ifdef HAVE_PYTHON

/*
 * Calls the bjam rule specified by name passed in 'args'. The name is looked up
 * in the context of bjam's 'python_interface' module. Returns the list of
 * strings returned by the rule.
 */

PyObject * bjam_call( PyObject * self, PyObject * args )
{
    FRAME    inner[ 1 ];
    LIST   * result;
    PARSE  * p;
    OBJECT * rulename;

    /* Build up the list of arg lists. */
    frame_init( inner );
    inner->prev = 0;
    inner->prev_user = 0;
    inner->module = bindmodule( constant_python_interface );

    /* Extract the rule name and arguments from 'args'. */

    /* PyTuple_GetItem returns borrowed reference. */
    rulename = object_new( PyString_AsString( PyTuple_GetItem( args, 0 ) ) );
    {
        int i = 1;
        int size = PyTuple_Size( args );
        for ( ; i < size; ++i )
        {
            PyObject * a = PyTuple_GetItem( args, i );
            if ( PyString_Check( a ) )
            {
                lol_add( inner->args, list_new( object_new(
                    PyString_AsString( a ) ) ) );
            }
            else if ( PySequence_Check( a ) )
            {
                LIST * l = 0;
                int s = PySequence_Size( a );
                int i = 0;
                for ( ; i < s; ++i )
                {
                    /* PySequence_GetItem returns new reference. */
                    PyObject * e = PySequence_GetItem( a, i );
                    char * s = PyString_AsString( e );
                    if ( !s )
                    {
                        printf( "Invalid parameter type passed from Python\n" );
                        exit( 1 );
                    }
                    l = list_push_back( l, object_new( s ) );
                    Py_DECREF( e );
                }
                lol_add( inner->args, l );
            }
        }
    }

    result = evaluate_rule( bindrule( rulename, inner->module), rulename, inner );
    object_free( rulename );

    frame_free( inner );

    /* Convert the bjam list into a Python list result. */
    {
        PyObject * const pyResult = PyList_New( list_length( result ) );
        int i = 0;
        LISTITER iter = list_begin( result );
        LISTITER const end = list_end( result );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            PyList_SetItem( pyResult, i, PyString_FromString( object_str(
                list_item( iter ) ) ) );
            i += 1;
        }
        list_free( result );
        return pyResult;
    }
}


/*
 * Accepts four arguments:
 * - module name
 * - rule name,
 * - Python callable.
 * - (optional) bjam language function signature.
 * Creates a bjam rule with the specified name in the specified module, which
 * will invoke the Python callable.
 */

PyObject * bjam_import_rule( PyObject * self, PyObject * args )
{
    char     * module;
    char     * rule;
    PyObject * func;
    PyObject * bjam_signature = NULL;
    module_t * m;
    RULE     * r;
    OBJECT   * module_name;
    OBJECT   * rule_name;

    if ( !PyArg_ParseTuple( args, "ssO|O:import_rule",
                            &module, &rule, &func, &bjam_signature ) )
        return NULL;

    if ( !PyCallable_Check( func ) )
    {
        PyErr_SetString( PyExc_RuntimeError, "Non-callable object passed to "
            "bjam.import_rule" );
        return NULL;
    }

    module_name = *module ? object_new( module ) : 0;
    m = bindmodule( module_name );
    if ( module_name )
        object_free( module_name );
    rule_name = object_new( rule );
    new_rule_body( m, rule_name, function_python( func, bjam_signature ), 0 );
    object_free( rule_name );

    Py_INCREF( Py_None );
    return Py_None;
}


/*
 * Accepts four arguments:
 *  - an action name
 *  - an action body
 *  - a list of variable that will be bound inside the action
 *  - integer flags.
 *  Defines an action on bjam side.
 */

PyObject * bjam_define_action( PyObject * self, PyObject * args )
{
    char     * name;
    char     * body;
    module_t * m;
    PyObject * bindlist_python;
    int        flags;
    LIST     * bindlist = L0;
    int        n;
    int        i;
    OBJECT   * name_str;
    FUNCTION * body_func;

    if ( !PyArg_ParseTuple( args, "ssO!i:define_action", &name, &body,
        &PyList_Type, &bindlist_python, &flags ) )
        return NULL;

    n = PyList_Size( bindlist_python );
    for ( i = 0; i < n; ++i )
    {
        PyObject * next = PyList_GetItem( bindlist_python, i );
        if ( !PyString_Check( next ) )
        {
            PyErr_SetString( PyExc_RuntimeError, "bind list has non-string "
                "type" );
            return NULL;
        }
        bindlist = list_push_back( bindlist, object_new( PyString_AsString( next
            ) ) );
    }

    name_str = object_new( name );
    body_func = function_compile_actions( body, constant_builtin, -1 );
    new_rule_actions( root_module(), name_str, body_func, bindlist, flags );
    function_free( body_func );
    object_free( name_str );

    Py_INCREF( Py_None );
    return Py_None;
}


/*
 * Returns the value of a variable in root Jam module.
 */

PyObject * bjam_variable( PyObject * self, PyObject * args )
{
    char     * name;
    LIST     * value;
    PyObject * result;
    int        i;
    OBJECT   * varname;
    LISTITER   iter;
    LISTITER   end;

    if ( !PyArg_ParseTuple( args, "s", &name ) )
        return NULL;

    varname = object_new( name );
    value = var_get( root_module(), varname );
    object_free( varname );
    iter = list_begin( value );
    end = list_end( value );

    result = PyList_New( list_length( value ) );
    for ( i = 0; iter != end; iter = list_next( iter ), ++i )
        PyList_SetItem( result, i, PyString_FromString( object_str( list_item(
            iter ) ) ) );

    return result;
}


PyObject * bjam_backtrace( PyObject * self, PyObject * args )
{
    PyObject     * result = PyList_New( 0 );
    struct frame * f = frame_before_python_call;

    for ( ; f = f->prev; )
    {
        PyObject   * tuple = PyTuple_New( 4 );
        char const * file;
        int          line;
        char         buf[ 32 ];
        string module_name[ 1 ];

        get_source_line( f, &file, &line );
        sprintf( buf, "%d", line );
        string_new( module_name );
        if ( f->module->name )
        {
            string_append( module_name, object_str( f->module->name ) );
            string_append( module_name, "." );
        }

        /* PyTuple_SetItem steals reference. */
        PyTuple_SetItem( tuple, 0, PyString_FromString( file ) );
        PyTuple_SetItem( tuple, 1, PyString_FromString( buf ) );
        PyTuple_SetItem( tuple, 2, PyString_FromString( module_name->value ) );
        PyTuple_SetItem( tuple, 3, PyString_FromString( f->rulename ) );

        string_free( module_name );

        PyList_Append( result, tuple );
        Py_DECREF( tuple );
    }
    return result;
}

PyObject * bjam_caller( PyObject * self, PyObject * args )
{
    return PyString_FromString( frame_before_python_call->prev->module->name ?
        object_str( frame_before_python_call->prev->module->name ) : "" );
}

#endif  /* #ifdef HAVE_PYTHON */


#ifdef HAVE_POPEN

#if defined(_MSC_VER) || defined(__BORLANDC__)
    #define popen windows_popen_wrapper
    #define pclose _pclose

    /*
     * This wrapper is a workaround for a funny _popen() feature on Windows
     * where it eats external quotes in some cases. The bug seems to be related
     * to the quote stripping functionality used by the Windows cmd.exe
     * interpreter when its /S is not specified.
     *
     * Cleaned up quote from the cmd.exe help screen as displayed on Windows XP
     * SP3:
     *
     *   1. If all of the following conditions are met, then quote characters on
     *      the command line are preserved:
     *
     *       - no /S switch
     *       - exactly two quote characters
     *       - no special characters between the two quote characters, where
     *         special is one of: &<>()@^|
     *       - there are one or more whitespace characters between the two quote
     *         characters
     *       - the string between the two quote characters is the name of an
     *         executable file.
     *
     *   2. Otherwise, old behavior is to see if the first character is a quote
     *      character and if so, strip the leading character and remove the last
     *      quote character on the command line, preserving any text after the
     *      last quote character.
     *
     * This causes some commands containing quotes not to be executed correctly.
     * For example:
     *
     *   "\Long folder name\aaa.exe" --name="Jurko" --no-surname
     *
     * would get its outermost quotes stripped and would be executed as:
     *
     *   \Long folder name\aaa.exe" --name="Jurko --no-surname
     *
     * which would report an error about '\Long' not being a valid command.
     *
     * cmd.exe help seems to indicate it would be enough to add an extra space
     * character in front of the command to avoid this but this does not work,
     * most likely due to the shell first stripping all leading whitespace
     * characters from the command.
     *
     * Solution implemented here is to quote the whole command in case it
     * contains any quote characters. Note thought this will not work correctly
     * should Windows ever 'fix' this feature.
     *                                               (03.06.2008.) (Jurko)
     */
    static FILE * windows_popen_wrapper( char const * command,
        char const * mode )
    {
        int const extra_command_quotes_needed = !!strchr( command, '"' );
        string quoted_command;
        FILE * result;

        if ( extra_command_quotes_needed )
        {
            string_new( &quoted_command );
            string_append( &quoted_command, "\"" );
            string_append( &quoted_command, command );
            string_append( &quoted_command, "\"" );
            command = quoted_command.value;
        }

        result = _popen( command, "r" );

        if ( extra_command_quotes_needed )
            string_free( &quoted_command );

        return result;
    }
#endif  /* defined(_MSC_VER) || defined(__BORLANDC__) */


static char * rtrim( char * const s )
{
    char * p = s;
    while ( *p ) ++p;
    for ( --p; p >= s && isspace( *p ); *p-- = 0 );
    return s;
}


LIST * builtin_shell( FRAME * frame, int flags )
{
    LIST   * command = lol_get( frame->args, 0 );
    LIST   * result = L0;
    string   s;
    int      ret;
    char     buffer[ 1024 ];
    FILE   * p = NULL;
    int      exit_status = -1;
    int      exit_status_opt = 0;
    int      no_output_opt = 0;
    int      strip_eol_opt = 0;

    /* Process the variable args options. */
    {
        int a = 1;
        LIST * arg = lol_get( frame->args, a );
        for ( ; !list_empty( arg ); arg = lol_get( frame->args, ++a ) )
        {
            if ( !strcmp( "exit-status", object_str( list_front( arg ) ) ) )
                exit_status_opt = 1;
            else if ( !strcmp( "no-output", object_str( list_front( arg ) ) ) )
                no_output_opt = 1;
            else if ( !strcmp("strip-eol", object_str( list_front( arg ) ) ) )
                strip_eol_opt = 1;
        }
    }

    /* The following fflush() call seems to be indicated as a workaround for a
     * popen() bug on POSIX implementations related to synhronizing input
     * stream positions for the called and the calling process.
     */
    fflush( NULL );

    p = popen( object_str( list_front( command ) ), "r" );
    if ( p == NULL )
        return L0;

    string_new( &s );

    while ( ( ret = fread( buffer, sizeof( char ), sizeof( buffer ) - 1, p ) ) >
        0 )
    {
        buffer[ ret ] = 0;
        if ( !no_output_opt )
        {
            if ( strip_eol_opt )
                rtrim( buffer );
            string_append( &s, buffer );
        }
    }

    exit_status = pclose( p );

    /* The command output is returned first. */
    result = list_new( object_new( s.value ) );
    string_free( &s );

    /* The command exit result next. */
    if ( exit_status_opt )
    {
        if ( WIFEXITED( exit_status ) )
            exit_status = WEXITSTATUS( exit_status );
        else
            exit_status = -1;
        sprintf( buffer, "%d", exit_status );
        result = list_push_back( result, object_new( buffer ) );
    }

    return result;
}

#else  /* #ifdef HAVE_POPEN */

LIST * builtin_shell( FRAME * frame, int flags )
{
    return L0;
}

#endif  /* #ifdef HAVE_POPEN */
