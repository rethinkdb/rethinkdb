/*
 * Copyright 2011 Steven Watanabe
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "jam.h"
#include "function.h"

#include "class.h"
#include "compile.h"
#include "constants.h"
#include "filesys.h"
#include "frames.h"
#include "lists.h"
#include "mem.h"
#include "pathsys.h"
#include "rules.h"
#include "search.h"
#include "variable.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS_CYGWIN
# include <cygwin/version.h>
# include <sys/cygwin.h>
# ifdef CYGWIN_VERSION_CYGWIN_CONV
#  include <errno.h>
# endif
# include <windows.h>
#endif

int glob( char const * s, char const * c );
void backtrace( FRAME * );
void backtrace_line( FRAME * );

#define INSTR_PUSH_EMPTY                   0
#define INSTR_PUSH_CONSTANT                1
#define INSTR_PUSH_ARG                     2
#define INSTR_PUSH_VAR                     3
#define INSTR_PUSH_VAR_FIXED               57
#define INSTR_PUSH_GROUP                   4
#define INSTR_PUSH_RESULT                  5
#define INSTR_PUSH_APPEND                  6
#define INSTR_SWAP                         7

#define INSTR_JUMP_EMPTY                   8
#define INSTR_JUMP_NOT_EMPTY               9

#define INSTR_JUMP                         10
#define INSTR_JUMP_LT                      11
#define INSTR_JUMP_LE                      12
#define INSTR_JUMP_GT                      13
#define INSTR_JUMP_GE                      14
#define INSTR_JUMP_EQ                      15
#define INSTR_JUMP_NE                      16
#define INSTR_JUMP_IN                      17
#define INSTR_JUMP_NOT_IN                  18

#define INSTR_JUMP_NOT_GLOB                19

#define INSTR_FOR_INIT                     56
#define INSTR_FOR_LOOP                     20

#define INSTR_SET_RESULT                   21
#define INSTR_RETURN                       22
#define INSTR_POP                          23

#define INSTR_PUSH_LOCAL                   24
#define INSTR_POP_LOCAL                    25
#define INSTR_SET                          26
#define INSTR_APPEND                       27
#define INSTR_DEFAULT                      28

#define INSTR_PUSH_LOCAL_FIXED             58
#define INSTR_POP_LOCAL_FIXED              59
#define INSTR_SET_FIXED                    60
#define INSTR_APPEND_FIXED                 61
#define INSTR_DEFAULT_FIXED                62

#define INSTR_PUSH_LOCAL_GROUP             29
#define INSTR_POP_LOCAL_GROUP              30
#define INSTR_SET_GROUP                    31
#define INSTR_APPEND_GROUP                 32
#define INSTR_DEFAULT_GROUP                33

#define INSTR_PUSH_ON                      34
#define INSTR_POP_ON                       35
#define INSTR_SET_ON                       36
#define INSTR_APPEND_ON                    37
#define INSTR_DEFAULT_ON                   38
#define INSTR_GET_ON                       65

#define INSTR_CALL_RULE                    39
#define INSTR_CALL_MEMBER_RULE             66

#define INSTR_APPLY_MODIFIERS              40
#define INSTR_APPLY_INDEX                  41
#define INSTR_APPLY_INDEX_MODIFIERS        42
#define INSTR_APPLY_MODIFIERS_GROUP        43
#define INSTR_APPLY_INDEX_GROUP            44
#define INSTR_APPLY_INDEX_MODIFIERS_GROUP  45
#define INSTR_COMBINE_STRINGS              46
#define INSTR_GET_GRIST                    64

#define INSTR_INCLUDE                      47
#define INSTR_RULE                         48
#define INSTR_ACTIONS                      49
#define INSTR_PUSH_MODULE                  50
#define INSTR_POP_MODULE                   51
#define INSTR_CLASS                        52
#define INSTR_BIND_MODULE_VARIABLES        63

#define INSTR_APPEND_STRINGS               53
#define INSTR_WRITE_FILE                   54
#define INSTR_OUTPUT_STRINGS               55

typedef struct instruction
{
    unsigned int op_code;
    int arg;
} instruction;

typedef struct _subfunction
{
    OBJECT * name;
    FUNCTION * code;
    int local;
} SUBFUNCTION;

typedef struct _subaction
{
    OBJECT * name;
    FUNCTION * command;
    int flags;
} SUBACTION;

#define FUNCTION_BUILTIN    0
#define FUNCTION_JAM        1

struct argument
{
    int flags;
#define ARG_ONE 0
#define ARG_OPTIONAL 1
#define ARG_PLUS 2
#define ARG_STAR 3
#define ARG_VARIADIC 4
    OBJECT * type_name;
    OBJECT * arg_name;
    int index;
};

struct arg_list
{
    int size;
    struct argument * args;
};

struct _function
{
    int type;
    int reference_count;
    OBJECT * rulename;
    struct arg_list * formal_arguments;
    int num_formal_arguments;
};

typedef struct _builtin_function
{
    FUNCTION base;
    LIST * ( * func )( FRAME *, int flags );
    int flags;
} BUILTIN_FUNCTION;

typedef struct _jam_function
{
    FUNCTION base;
    int code_size;
    instruction * code;
    int num_constants;
    OBJECT * * constants;
    int num_subfunctions;
    SUBFUNCTION * functions;
    int num_subactions;
    SUBACTION * actions;
    FUNCTION * generic;
    OBJECT * file;
    int line;
} JAM_FUNCTION;


#ifdef HAVE_PYTHON

#define FUNCTION_PYTHON     2

typedef struct _python_function
{
    FUNCTION base;
    PyObject * python_function;
} PYTHON_FUNCTION;

static LIST * call_python_function( PYTHON_FUNCTION *, FRAME * );

#endif


struct _stack
{
    void * data;
};

static void * stack;

STACK * stack_global()
{
    static STACK result;
    if ( !stack )
    {
        int const size = 1 << 21;
        stack = BJAM_MALLOC( size );
        result.data = (char *)stack + size;
    }
    return &result;
}

static void check_alignment( STACK * s )
{
    assert( (size_t)s->data % sizeof( LIST * ) == 0 );
}

void * stack_allocate( STACK * s, int size )
{
    check_alignment( s );
    s->data = (char *)s->data - size;
    check_alignment( s );
    return s->data;
}

void stack_deallocate( STACK * s, int size )
{
    check_alignment( s );
    s->data = (char *)s->data + size;
    check_alignment( s );
}

void stack_push( STACK * s, LIST * l )
{
    *(LIST * *)stack_allocate( s, sizeof( LIST * ) ) = l;
}

LIST * stack_pop( STACK * s )
{
    LIST * const result = *(LIST * *)s->data;
    stack_deallocate( s, sizeof( LIST * ) );
    return result;
}

LIST * stack_top( STACK * s )
{
    check_alignment( s );
    return *(LIST * *)s->data;
}

LIST * stack_at( STACK * s, int n )
{
    check_alignment( s );
    return *( (LIST * *)s->data + n );
}

void stack_set( STACK * s, int n, LIST * value )
{
    check_alignment( s );
    *((LIST * *)s->data + n) = value;
}

void * stack_get( STACK * s )
{
    check_alignment( s );
    return s->data;
}

LIST * frame_get_local( FRAME * frame, int idx )
{
    /* The only local variables are the arguments. */
    return list_copy( lol_get( frame->args, idx ) );
}

static OBJECT * function_get_constant( JAM_FUNCTION * function, int idx )
{
    return function->constants[ idx ];
}

static LIST * function_get_variable( JAM_FUNCTION * function, FRAME * frame,
    int idx )
{
    return list_copy( var_get( frame->module, function->constants[ idx ] ) );
}

static void function_set_variable( JAM_FUNCTION * function, FRAME * frame,
    int idx, LIST * value )
{
    var_set( frame->module, function->constants[ idx ], value, VAR_SET );
}

static LIST * function_swap_variable( JAM_FUNCTION * function, FRAME * frame,
    int idx, LIST * value )
{
    return var_swap( frame->module, function->constants[ idx ], value );
}

static void function_append_variable( JAM_FUNCTION * function, FRAME * frame,
    int idx, LIST * value )
{
    var_set( frame->module, function->constants[ idx ], value, VAR_APPEND );
}

static void function_default_variable( JAM_FUNCTION * function, FRAME * frame,
    int idx, LIST * value )
{
    var_set( frame->module, function->constants[ idx ], value, VAR_DEFAULT );
}

static void function_set_rule( JAM_FUNCTION * function, FRAME * frame,
    STACK * s, int idx )
{
    SUBFUNCTION * sub = function->functions + idx;
    new_rule_body( frame->module, sub->name, sub->code, !sub->local );
}

static void function_set_actions( JAM_FUNCTION * function, FRAME * frame,
    STACK * s, int idx )
{
    SUBACTION * sub = function->actions + idx;
    LIST * bindlist = stack_pop( s );
    new_rule_actions( frame->module, sub->name, sub->command, bindlist,
        sub->flags );
}


/*
 * Returns the index if name is "<", ">", "1", "2", ... or "19" otherwise
 * returns -1.
 */

static int get_argument_index( char const * s )
{
    if ( s[ 0 ] != '\0')
    {
        if ( s[ 1 ] == '\0' )
        {
            switch ( s[ 0 ] )
            {
                case '<': return 0;
                case '>': return 1;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    return s[ 0 ] - '1';
            }
        }
        else if ( s[ 0 ] == '1' && s[ 2 ] == '\0' )
        {
            switch( s[ 1 ] )
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return s[ 1 ] - '0' + 10 - 1;
            }
        }
    }
    return -1;
}

static LIST * function_get_named_variable( JAM_FUNCTION * function,
    FRAME * frame, OBJECT * name )
{
    int const idx = get_argument_index( object_str( name ) );
    return idx == -1
        ? list_copy( var_get( frame->module, name ) )
        : list_copy( lol_get( frame->args, idx ) );
}

static void function_set_named_variable( JAM_FUNCTION * function, FRAME * frame,
    OBJECT * name, LIST * value)
{
    var_set( frame->module, name, value, VAR_SET );
}

static LIST * function_swap_named_variable( JAM_FUNCTION * function,
    FRAME * frame, OBJECT * name, LIST * value )
{
    return var_swap( frame->module, name, value );
}

static void function_append_named_variable( JAM_FUNCTION * function,
    FRAME * frame, OBJECT * name, LIST * value)
{
    var_set( frame->module, name, value, VAR_APPEND );
}

static void function_default_named_variable( JAM_FUNCTION * function,
    FRAME * frame, OBJECT * name, LIST * value )
{
    var_set( frame->module, name, value, VAR_DEFAULT );
}

static LIST * function_call_rule( JAM_FUNCTION * function, FRAME * frame,
    STACK * s, int n_args, char const * unexpanded, OBJECT * file, int line )
{
    FRAME inner[ 1 ];
    int i;
    LIST * first = stack_pop( s );
    LIST * result = L0;
    OBJECT * rulename;
    LIST * trailing;

    frame->file = file;
    frame->line = line;

    if ( list_empty( first ) )
    {
        backtrace_line( frame );
        printf( "warning: rulename %s expands to empty string\n", unexpanded );
        backtrace( frame );
        list_free( first );
        for ( i = 0; i < n_args; ++i )
            list_free( stack_pop( s ) );
        return result;
    }

    rulename = object_copy( list_front( first ) );

    frame_init( inner );
    inner->prev = frame;
    inner->prev_user = frame->module->user_module ? frame : frame->prev_user;
    inner->module = frame->module;  /* This gets fixed up in evaluate_rule(). */

    for ( i = 0; i < n_args; ++i )
        lol_add( inner->args, stack_at( s, n_args - i - 1 ) );

    for ( i = 0; i < n_args; ++i )
        stack_pop( s );

    trailing = list_pop_front( first );
    if ( trailing )
    {
        if ( inner->args->count == 0 )
            lol_add( inner->args, trailing );
        else
        {
            LIST * * const l = &inner->args->list[ 0 ];
            *l = list_append( trailing, *l );
        }
    }

    result = evaluate_rule( bindrule( rulename, inner->module ), rulename, inner );
    frame_free( inner );
    object_free( rulename );
    return result;
}

static LIST * function_call_member_rule( JAM_FUNCTION * function, FRAME * frame, STACK * s, int n_args, OBJECT * rulename, OBJECT * file, int line )
{
    FRAME   inner[ 1 ];
    int i;
    LIST * first = stack_pop( s );
    LIST * result = L0;
    LIST * trailing;
    RULE * rule;
    module_t * module;
    OBJECT * real_rulename = 0;

    frame->file = file;
    frame->line = line;
    
    if ( list_empty( first ) )
    {
        backtrace_line( frame );
        printf( "warning: object is empty\n" );
        backtrace( frame );

        list_free( first );

        for( i = 0; i < n_args; ++i )
        {
            list_free( stack_pop( s ) );
        }

        return result;
    }

    /* FIXME: handle generic case */
    assert( list_length( first ) == 1 );

    module = bindmodule( list_front( first ) );
    if ( module->class_module )
    {
        rule = bindrule( rulename, module );
        real_rulename = object_copy( function_rulename( rule->procedure ) );
    }
    else
    {
        string buf[ 1 ];
        string_new( buf );
        string_append( buf, object_str( list_front( first ) ) );
        string_push_back( buf, '.' );
        string_append( buf, object_str( rulename ) );
        real_rulename = object_new( buf->value );
        string_free( buf );
        rule = bindrule( real_rulename, frame->module );
    }

    frame_init( inner );

    inner->prev = frame;
    inner->prev_user = frame->module->user_module ? frame : frame->prev_user;
    inner->module = frame->module;  /* This gets fixed up in evaluate_rule(), below. */

    for( i = 0; i < n_args; ++i )
    {
        lol_add( inner->args, stack_at( s, n_args - i - 1 ) );
    }

    for( i = 0; i < n_args; ++i )
    {
        stack_pop( s );
    }

    if ( list_length( first ) > 1 )
    {
        string buf[ 1 ];
        LIST * trailing = L0;
        LISTITER iter = list_begin( first ), end = list_end( first );
        iter = list_next( iter );
        string_new( buf );
        for ( ; iter != end; iter = list_next( iter ) )
        {
            string_append( buf, object_str( list_item( iter ) ) );
            string_push_back( buf, '.' );
            string_append( buf, object_str( rulename ) );
            trailing = list_push_back( trailing, object_new( buf->value ) );
            string_truncate( buf, 0 );
        }
        string_free( buf );
        if ( inner->args->count == 0 )
            lol_add( inner->args, trailing );
        else
        {
            LIST * * const l = &inner->args->list[ 0 ];
            *l = list_append( trailing, *l );
        }
    }

    result = evaluate_rule( rule, real_rulename, inner );
    frame_free( inner );
    object_free( rulename );
    object_free( real_rulename );
    return result;
}


/* Variable expansion */

typedef struct
{
    int sub1;
    int sub2;
} subscript_t;

typedef struct
{
    PATHNAME f;           /* :GDBSMR -- pieces */
    char     parent;      /* :P -- go to parent directory */
    char     filemods;    /* one of the above applied */
    char     downshift;   /* :L -- downshift result */
    char     upshift;     /* :U -- upshift result */
    char     to_slashes;  /* :T -- convert "\" to "/" */
    char     to_windows;  /* :W -- convert cygwin to native paths */
    PATHPART empty;       /* :E -- default for empties */
    PATHPART join;        /* :J -- join list with char */
} VAR_EDITS;

static LIST * apply_modifiers_impl( LIST * result, string * buf,
    VAR_EDITS * edits, int n, LISTITER iter, LISTITER end );
static void get_iters( subscript_t const subscript, LISTITER * const first,
    LISTITER * const last, int const length );


/*
 * var_edit_parse() - parse : modifiers into PATHNAME structure
 *
 * The : modifiers in a $(varname:modifier) currently support replacing or
 * omitting elements of a filename, and so they are parsed into a PATHNAME
 * structure (which contains pointers into the original string).
 *
 * Modifiers of the form "X=value" replace the component X with the given value.
 * Modifiers without the "=value" cause everything but the component X to be
 * omitted. X is one of:
 *
 *  G <grist>
 *  D directory name
 *  B base name
 *  S .suffix
 *  M (member)
 *  R root directory - prepended to whole path
 *
 * This routine sets:
 *
 *  f->f_xxx.ptr = 0
 *  f->f_xxx.len = 0
 *      -> leave the original component xxx
 *
 *  f->f_xxx.ptr = string
 *  f->f_xxx.len = strlen( string )
 *      -> replace component xxx with string
 *
 *  f->f_xxx.ptr = ""
 *  f->f_xxx.len = 0
 *      -> omit component xxx
 *
 * var_edit_file() below and path_build() obligingly follow this convention.
 */

static int var_edit_parse( char const * mods, VAR_EDITS * edits, int havezeroed
    )
{
    while ( *mods )
    {
        PATHPART * fp;

        switch ( *mods++ )
        {
            case 'L': edits->downshift = 1; continue;
            case 'U': edits->upshift = 1; continue;
            case 'P': edits->parent = edits->filemods = 1; continue;
            case 'E': fp = &edits->empty; goto strval;
            case 'J': fp = &edits->join; goto strval;
            case 'G': fp = &edits->f.f_grist; goto fileval;
            case 'R': fp = &edits->f.f_root; goto fileval;
            case 'D': fp = &edits->f.f_dir; goto fileval;
            case 'B': fp = &edits->f.f_base; goto fileval;
            case 'S': fp = &edits->f.f_suffix; goto fileval;
            case 'M': fp = &edits->f.f_member; goto fileval;
            case 'T': edits->to_slashes = 1; continue;
            case 'W': edits->to_windows = 1; continue;
            default:
                continue;  /* Should complain, but so what... */
        }

    fileval:
        /* Handle :CHARS, where each char (without a following =) selects a
         * particular file path element. On the first such char, we deselect all
         * others (by setting ptr = "", len = 0) and for each char we select
         * that element (by setting ptr = 0).
         */
        edits->filemods = 1;

        if ( *mods != '=' )
        {
            if ( !havezeroed++ )
            {
                int i;
                for ( i = 0; i < 6; ++i )
                {
                    edits->f.part[ i ].len = 0;
                    edits->f.part[ i ].ptr = "";
                }
            }

            fp->ptr = 0;
            continue;
        }

    strval:
        /* Handle :X=value, or :X */
        if ( *mods != '=' )
        {
            fp->ptr = "";
            fp->len = 0;
        }
        else
        {
            fp->ptr = ++mods;
            fp->len = strlen( mods );
            mods += fp->len;
        }
    }

    return havezeroed;
}


/*
 * var_edit_file() - copy input target name to output, modifying filename.
 */

static void var_edit_file( char const * in, string * out, VAR_EDITS * edits )
{
    if ( edits->filemods )
    {
        PATHNAME pathname;

        /* Parse apart original filename, putting parts into "pathname". */
        path_parse( in, &pathname );

        /* Replace any pathname with edits->f */
        if ( edits->f.f_grist .ptr ) pathname.f_grist  = edits->f.f_grist;
        if ( edits->f.f_root  .ptr ) pathname.f_root   = edits->f.f_root;
        if ( edits->f.f_dir   .ptr ) pathname.f_dir    = edits->f.f_dir;
        if ( edits->f.f_base  .ptr ) pathname.f_base   = edits->f.f_base;
        if ( edits->f.f_suffix.ptr ) pathname.f_suffix = edits->f.f_suffix;
        if ( edits->f.f_member.ptr ) pathname.f_member = edits->f.f_member;

        /* If requested, modify pathname to point to parent. */
        if ( edits->parent )
            path_parent( &pathname );

        /* Put filename back together. */
        path_build( &pathname, out );
    }
    else
        string_append( out, in );
}

/*
 * var_edit_cyg2win() - conversion of a cygwin to a Windows path.
 *
 * FIXME: skip grist
 */

#ifdef OS_CYGWIN
static void var_edit_cyg2win( string * out, size_t pos, VAR_EDITS * edits )
{
    if ( edits->to_windows )
    {
    #ifdef CYGWIN_VERSION_CYGWIN_CONV
        /* Use new Cygwin API added with Cygwin 1.7. Old one had no error
         * handling and has been deprecated.
         */
        char * dynamicBuffer = 0;
        char buffer[ MAX_PATH + 1001 ];
        char const * result = buffer;
        cygwin_conv_path_t const conv_type = CCP_POSIX_TO_WIN_A | CCP_RELATIVE;
        ssize_t const apiResult = cygwin_conv_path( conv_type, out->value + pos,
            buffer, sizeof( buffer ) / sizeof( *buffer ) );
        assert( apiResult == 0 || apiResult == -1 );
        assert( apiResult || strlen( result ) < sizeof( buffer ) / sizeof(
            *buffer ) );
        if ( apiResult )
        {
            result = 0;
            if ( errno == ENOSPC )
            {
                ssize_t const size = cygwin_conv_path( conv_type, out->value +
                    pos, NULL, 0 );
                assert( size >= -1 );
                if ( size > 0 )
                {
                    dynamicBuffer = (char *)BJAM_MALLOC_ATOMIC( size );
                    if ( dynamicBuffer )
                    {
                        ssize_t const apiResult = cygwin_conv_path( conv_type,
                            out->value + pos, dynamicBuffer, size );
                        assert( apiResult == 0 || apiResult == -1 );
                        if ( !apiResult )
                        {
                            result = dynamicBuffer;
                            assert( strlen( result ) < size );
                        }
                    }
                }
            }
        }
    #else  /* CYGWIN_VERSION_CYGWIN_CONV */
        /* Use old Cygwin API deprecated with Cygwin 1.7. */
        char result[ MAX_PATH + 1 ];
        cygwin_conv_to_win32_path( out->value + pos, result );
        assert( strlen( result ) <= MAX_PATH );
    #endif  /* CYGWIN_VERSION_CYGWIN_CONV */
        if ( result )
        {
            string_truncate( out, pos );
            string_append( out, result );
            edits->to_slashes = 0;
        }
    #ifdef CYGWIN_VERSION_CYGWIN_CONV
        if ( dynamicBuffer )
            BJAM_FREE( dynamicBuffer );
    #endif
    }
}
#endif  /* OS_CYGWIN */


/*
 * var_edit_shift() - do upshift/downshift & other mods.
 */

static void var_edit_shift( string * out, size_t pos, VAR_EDITS * edits )
{
#ifdef OS_CYGWIN
    var_edit_cyg2win( out, pos, edits );
#endif

    if ( edits->upshift || edits->downshift || edits->to_slashes )
    {
        /* Handle upshifting, downshifting and slash translation now. */
        char * p;
        for ( p = out->value + pos; *p; ++p )
        {
            if ( edits->upshift )
                *p = toupper( *p );
            else if ( edits->downshift )
                *p = tolower( *p );
            if ( edits->to_slashes && ( *p == '\\' ) )
                *p = '/';
        }
    }
}


/*
 * Reads n LISTs from the top of the STACK and combines them to form VAR_EDITS.
 * Returns the number of VAR_EDITS pushed onto the STACK.
 */

static int expand_modifiers( STACK * s, int n )
{
    int i;
    int total = 1;
    LIST * * args = stack_get( s );
    for ( i = 0; i < n; ++i )
        total *= list_length( args[ i ] );

    if ( total != 0 )
    {
        VAR_EDITS * out = stack_allocate( s, total * sizeof( VAR_EDITS ) );
        LISTITER * iter = stack_allocate( s, n * sizeof( LIST * ) );
        for ( i = 0; i < n; ++i )
            iter[ i ] = list_begin( args[ i ] );
        i = 0;
        {
            int havezeroed;
        loop:
            memset( out, 0, sizeof( *out ) );
            havezeroed = 0;
            for ( i = 0; i < n; ++i )
                havezeroed = var_edit_parse( object_str( list_item( iter[ i ] )
                    ), out, havezeroed );
            ++out;
            while ( --i >= 0 )
            {
                if ( list_next( iter[ i ] ) != list_end( args[ i ] ) )
                {
                    iter[ i ] = list_next( iter[ i ] );
                    goto loop;
                }
                iter[ i ] = list_begin( args[ i ] );
            }
        }
        stack_deallocate( s, n * sizeof( LIST * ) );
    }
    return total;
}

static LIST * apply_modifiers( STACK * s, int n )
{
    LIST * value = stack_top( s );
    LIST * result = L0;
    VAR_EDITS * const edits = (VAR_EDITS *)( (LIST * *)stack_get( s ) + 1 );
    string buf[ 1 ];
    string_new( buf );
    result = apply_modifiers_impl( result, buf, edits, n, list_begin( value ),
        list_end( value ) );
    string_free( buf );
    return result;
}


/*
 * Parse a string of the form "1-2", "-2--1", "2-" and return the two
 * subscripts.
 */

subscript_t parse_subscript( char const * s )
{
    subscript_t result;
    result.sub1 = 0;
    result.sub2 = 0;
    do  /* so we can use "break" */
    {
        /* Allow negative subscripts. */
        if ( !isdigit( *s ) && ( *s != '-' ) )
        {
            result.sub2 = 0;
            break;
        }
        result.sub1 = atoi( s );

        /* Skip over the first symbol, which is either a digit or dash. */
        ++s;
        while ( isdigit( *s ) ) ++s;

        if ( *s == '\0' )
        {
            result.sub2 = result.sub1;
            break;
        }

        if ( *s != '-' )
        {
            result.sub2 = 0;
            break;
        }

        ++s;

        if ( *s == '\0' )
        {
            result.sub2 = -1;
            break;
        }

        if ( !isdigit( *s ) && ( *s != '-' ) )
        {
            result.sub2 = 0;
            break;
        }

        /* First, compute the index of the last element. */
        result.sub2 = atoi( s );
        while ( isdigit( *++s ) );

        if ( *s != '\0' )
            result.sub2 = 0;

    } while ( 0 );
    return result;
}

static LIST * apply_subscript( STACK * s )
{
    LIST * value = stack_top( s );
    LIST * indices = stack_at( s, 1 );
    LIST * result = L0;
    int length = list_length( value );
    string buf[ 1 ];
    LISTITER indices_iter = list_begin( indices );
    LISTITER const indices_end = list_end( indices );
    string_new( buf );
    for ( ; indices_iter != indices_end; indices_iter = list_next( indices_iter
        ) )
    {
        LISTITER iter = list_begin( value );
        LISTITER end = list_end( value );
        subscript_t const subscript = parse_subscript( object_str( list_item(
            indices_iter ) ) );
        get_iters( subscript, &iter, &end, length );
        for ( ; iter != end; iter = list_next( iter ) )
            result = list_push_back( result, object_copy( list_item( iter ) ) );
    }
    string_free( buf );
    return result;
}


/*
 * Reads the LIST from first and applies subscript to it. The results are
 * written to *first and *last.
 */

static void get_iters( subscript_t const subscript, LISTITER * const first,
    LISTITER * const last, int const length )
{
    int start;
    int size;
    LISTITER iter;
    LISTITER end;
    {

        if ( subscript.sub1 < 0 )
            start = length + subscript.sub1;
        else if ( subscript.sub1 > length )
            start = length;
        else
            start = subscript.sub1 - 1;

        size = subscript.sub2 < 0
            ? length + 1 + subscript.sub2 - start
            : subscript.sub2 - start;

        /*
         * HACK: When the first subscript is before the start of the list, it
         * magically becomes the beginning of the list. This is inconsistent,
         * but needed for backwards compatibility.
         */
        if ( start < 0 )
            start = 0;

        /* The "sub2 < 0" test handles the semantic error of sub2 < sub1. */
        if ( size < 0 )
            size = 0;

        if ( start + size > length )
            size = length - start;
    }

    iter = *first;
    while ( start-- > 0 )
        iter = list_next( iter );

    end = iter;
    while ( size-- > 0 )
        end = list_next( end );

    *first = iter;
    *last = end;
}

static LIST * apply_modifiers_empty( LIST * result, string * buf,
    VAR_EDITS * edits, int n )
{
    int i;
    for ( i = 0; i < n; ++i )
    {
        if ( edits[ i ].empty.ptr )
        {
            /** FIXME: is empty.ptr always null-terminated? */
            var_edit_file( edits[ i ].empty.ptr, buf, edits + i );
            var_edit_shift( buf, 0, edits + i );
            result = list_push_back( result, object_new( buf->value ) );
            string_truncate( buf, 0 );
        }
    }
    return result;
}

static LIST * apply_modifiers_non_empty( LIST * result, string * buf,
    VAR_EDITS * edits, int n, LISTITER begin, LISTITER end )
{
    int i;
    LISTITER iter;
    for ( i = 0; i < n; ++i )
    {
        if ( edits[ i ].join.ptr )
        {
            var_edit_file( object_str( list_item( begin ) ), buf, edits + i );
            var_edit_shift( buf, 0, edits + i );
            for ( iter = list_next( begin ); iter != end; iter = list_next( iter
                ) )
            {
                size_t size;
                string_append( buf, edits[ i ].join.ptr );
                size = buf->size;
                var_edit_file( object_str( list_item( iter ) ), buf, edits + i
                    );
                var_edit_shift( buf, size, edits + i );
            }
            result = list_push_back( result, object_new( buf->value ) );
            string_truncate( buf, 0 );
        }
        else
        {
            for ( iter = begin; iter != end; iter = list_next( iter ) )
            {
                var_edit_file( object_str( list_item( iter ) ), buf, edits + i );
                var_edit_shift( buf, 0, edits + i );
                result = list_push_back( result, object_new( buf->value ) );
                string_truncate( buf, 0 );
            }
        }
    }
    return result;
}

static LIST * apply_modifiers_impl( LIST * result, string * buf,
    VAR_EDITS * edits, int n, LISTITER iter, LISTITER end )
{
    return iter == end
        ? apply_modifiers_empty( result, buf, edits, n )
        : apply_modifiers_non_empty( result, buf, edits, n, iter, end );
}

static LIST * apply_subscript_and_modifiers( STACK * s, int n )
{
    LIST * const value = stack_top( s );
    LIST * const indices = stack_at( s, 1 );
    LIST * result = L0;
    VAR_EDITS * const edits = (VAR_EDITS *)((LIST * *)stack_get( s ) + 2);
    int const length = list_length( value );
    string buf[ 1 ];
    LISTITER indices_iter = list_begin( indices );
    LISTITER const indices_end = list_end( indices );
    string_new( buf );
    for ( ; indices_iter != indices_end; indices_iter = list_next( indices_iter
        ) )
    {
        LISTITER iter = list_begin( value );
        LISTITER end = list_end( value );
        subscript_t const sub = parse_subscript( object_str( list_item(
            indices_iter ) ) );
        get_iters( sub, &iter, &end, length );
        result = apply_modifiers_impl( result, buf, edits, n, iter, end );
    }
    string_free( buf );
    return result;
}


/*
 * expand() - expands a list of concatenated strings and variable refereces
 *
 * Takes a list of expansion items - each representing one element to be
 * concatenated and each containing a list of its values. Returns a list of all
 * possible values constructed by selecting a single value from each of the
 * elements and concatenating them together.
 *
 * For example, in the following code:
 *
 *     local a = one two three four ;
 *     local b = foo bar ;
 *     ECHO /$(a)/$(b)/$(a)/ ;
 *
 *   When constructing the result of /$(a)/$(b)/ this function would get called
 * with the following 7 expansion items:
 *     1. /
 *     2. one two three four
 *     3. /
 *     4. foo bar
 *     5. /
 *     6. one two three four
 *     7. /
 *
 *   And would result in a list containing 32 values:
 *     1. /one/foo/one/
 *     2. /one/foo/two/
 *     3. /one/foo/three/
 *     4. /one/foo/four/
 *     5. /one/bar/one/
 *     ...
 *
 */

typedef struct expansion_item
{
    /* Item's value list initialized prior to calling expand(). */
    LIST * values;

    /* Internal data initialized and used inside expand(). */
    LISTITER current;  /* Currently used value. */
    int size;          /* Concatenated string length prior to concatenating the
                        * item's current value.
                        */
} expansion_item;

static LIST * expand( expansion_item * items, int const length )
{
    LIST * result = L0;
    string buf[ 1 ];
    int size = 0;
    int i;

    assert( length > 0 );
    for ( i = 0; i < length; ++i )
    {
        LISTITER iter = list_begin( items[ i ].values );
        LISTITER const end = list_end( items[ i ].values );

        /* If any of the items has no values - the result is an empty list. */
        if ( iter == end ) return L0;

        /* Set each item's 'current' to its first listed value. This indicates
         * each item's next value to be used when constructing the list of all
         * possible concatenated values.
         */
        items[ i ].current = iter;

        /* Calculate the longest concatenated string length - to know how much
         * memory we need to allocate as a buffer for holding the concatenated
         * strings.
         */
        {
            int max = 0;
            for ( ; iter != end; iter = list_next( iter ) )
            {
                int const len = strlen( object_str( list_item( iter ) ) );
                if ( len > max ) max = len;
            }
            size += max;
        }
    }

    string_new( buf );
    string_reserve( buf, size );

    i = 0;
    while ( i >= 0 )
    {
        for ( ; i < length; ++i )
        {
            items[ i ].size = buf->size;
            string_append( buf, object_str( list_item( items[ i ].current ) ) );
        }
        result = list_push_back( result, object_new( buf->value ) );
        while ( --i >= 0 )
        {
            if ( list_next( items[ i ].current ) != list_end( items[ i ].values
                ) )
            {
                items[ i ].current = list_next( items[ i ].current );
                string_truncate( buf, items[ i ].size );
                break;
            }
            else
                items[ i ].current = list_begin( items[ i ].values );
        }
    }

    string_free( buf );
    return result;
}

static void combine_strings( STACK * s, int n, string * out )
{
    int i;
    for ( i = 0; i < n; ++i )
    {
        LIST * const values = stack_pop( s );
        LISTITER iter = list_begin( values );
        LISTITER const end = list_end( values );
        if ( iter != end )
        {
            string_append( out, object_str( list_item( iter ) ) );
            for ( iter = list_next( iter ); iter != end; iter = list_next( iter
                ) )
            {
                string_push_back( out, ' ' );
                string_append( out, object_str( list_item( iter ) ) );
            }
            list_free( values );
        }
    }
}

struct dynamic_array
{
    int size;
    int capacity;
    void * data;
};

static void dynamic_array_init( struct dynamic_array * array )
{
    array->size = 0;
    array->capacity = 0;
    array->data = 0;
}

static void dynamic_array_free( struct dynamic_array * array )
{
    BJAM_FREE( array->data );
}

static void dynamic_array_push_impl( struct dynamic_array * const array,
    void const * const value, int const unit_size )
{
    if ( array->capacity == 0 )
    {
        array->capacity = 2;
        array->data = BJAM_MALLOC( array->capacity * unit_size );
    }
    else if ( array->capacity == array->size )
    {
        void * new_data;
        array->capacity *= 2;
        new_data = BJAM_MALLOC( array->capacity * unit_size );
        memcpy( new_data, array->data, array->size * unit_size  );
        BJAM_FREE( array->data );
        array->data = new_data;
    }
    memcpy( (char *)array->data + array->size * unit_size, value, unit_size );
    ++array->size;
}

#define dynamic_array_push( array, value ) (dynamic_array_push_impl(array, &value, sizeof(value)))
#define dynamic_array_at( type, array, idx ) (((type *)(array)->data)[idx])


/*
 * struct compiler
 */

struct label_info
{
    int absolute_position;
    struct dynamic_array uses[ 1 ];
};

struct stored_rule
{
    OBJECT * name;
    PARSE * parse;
    int num_arguments;
    struct arg_list * arguments;
    int local;
};

typedef struct compiler
{
    struct dynamic_array code[ 1 ];
    struct dynamic_array constants[ 1 ];
    struct dynamic_array labels[ 1 ];
    struct dynamic_array rules[ 1 ];
    struct dynamic_array actions[ 1 ];
} compiler;

static void compiler_init( compiler * c )
{
    dynamic_array_init( c->code );
    dynamic_array_init( c->constants );
    dynamic_array_init( c->labels );
    dynamic_array_init( c->rules );
    dynamic_array_init( c->actions );
}

static void compiler_free( compiler * c )
{
    int i;
    dynamic_array_free( c->actions );
    dynamic_array_free( c->rules );
    for ( i = 0; i < c->labels->size; ++i )
        dynamic_array_free( dynamic_array_at( struct label_info, c->labels, i
            ).uses );
    dynamic_array_free( c->labels );
    dynamic_array_free( c->constants );
    dynamic_array_free( c->code );
}

static void compile_emit_instruction( compiler * c, instruction instr )
{
    dynamic_array_push( c->code, instr );
}

static int compile_new_label( compiler * c )
{
    int result = c->labels->size;
    struct label_info info;
    info.absolute_position = -1;
    dynamic_array_init( info.uses );
    dynamic_array_push( c->labels, info );
    return result;
}

static void compile_set_label( compiler * c, int label )
{
    struct label_info * const l = &dynamic_array_at( struct label_info,
        c->labels, label );
    int const pos = c->code->size;
    int i;
    assert( l->absolute_position == -1 );
    l->absolute_position = pos;
    for ( i = 0; i < l->uses->size; ++i )
    {
        int id = dynamic_array_at( int, l->uses, i );
        int offset = (int)( pos - id - 1 );
        dynamic_array_at( instruction, c->code, id ).arg = offset;
    }
}

static void compile_emit( compiler * c, unsigned int op_code, int arg )
{
    instruction instr;
    instr.op_code = op_code;
    instr.arg = arg;
    compile_emit_instruction( c, instr );
}

static void compile_emit_branch( compiler * c, unsigned int op_code, int label )
{
    struct label_info * const l = &dynamic_array_at( struct label_info,
        c->labels, label );
    int const pos = c->code->size;
    instruction instr;
    instr.op_code = op_code;
    if ( l->absolute_position == -1 )
    {
        instr.arg = 0;
        dynamic_array_push( l->uses, pos );
    }
    else
        instr.arg = (int)( l->absolute_position - pos - 1 );
    compile_emit_instruction( c, instr );
}

static int compile_emit_constant( compiler * c, OBJECT * value )
{
    OBJECT * copy = object_copy( value );
    dynamic_array_push( c->constants, copy );
    return c->constants->size - 1;
}

static int compile_emit_rule( compiler * c, OBJECT * name, PARSE * parse,
    int num_arguments, struct arg_list * arguments, int local )
{
    struct stored_rule rule;
    rule.name = object_copy( name );
    rule.parse = parse;
    rule.num_arguments = num_arguments;
    rule.arguments = arguments;
    rule.local = local;
    dynamic_array_push( c->rules, rule );
    return (int)( c->rules->size - 1 );
}

static int compile_emit_actions( compiler * c, PARSE * parse )
{
    SUBACTION a;
    a.name = object_copy( parse->string );
    a.command = function_compile_actions( object_str( parse->string1 ),
        parse->file, parse->line );
    a.flags = parse->num;
    dynamic_array_push( c->actions, a );
    return (int)( c->actions->size - 1 );
}

static JAM_FUNCTION * compile_to_function( compiler * c )
{
    JAM_FUNCTION * const result = BJAM_MALLOC( sizeof( JAM_FUNCTION ) );
    int i;
    result->base.type = FUNCTION_JAM;
    result->base.reference_count = 1;
    result->base.formal_arguments = 0;
    result->base.num_formal_arguments = 0;

    result->base.rulename = 0;

    result->code_size = c->code->size;
    result->code = BJAM_MALLOC( c->code->size * sizeof( instruction ) );
    memcpy( result->code, c->code->data, c->code->size * sizeof( instruction ) );

    result->constants = BJAM_MALLOC( c->constants->size * sizeof( OBJECT * ) );
    memcpy( result->constants, c->constants->data, c->constants->size * sizeof(
        OBJECT * ) );
    result->num_constants = c->constants->size;

    result->num_subfunctions = c->rules->size;
    result->functions = BJAM_MALLOC( c->rules->size * sizeof( SUBFUNCTION ) );
    for ( i = 0; i < c->rules->size; ++i )
    {
        struct stored_rule * const rule = &dynamic_array_at( struct stored_rule,
            c->rules, i );
        result->functions[ i ].name = rule->name;
        result->functions[ i ].code = function_compile( rule->parse );
        result->functions[ i ].code->num_formal_arguments = rule->num_arguments;
        result->functions[ i ].code->formal_arguments = rule->arguments;
        result->functions[ i ].local = rule->local;
    }

    result->actions = BJAM_MALLOC( c->actions->size * sizeof( SUBACTION ) );
    memcpy( result->actions, c->actions->data, c->actions->size * sizeof(
        SUBACTION ) );
    result->num_subactions = c->actions->size;

    result->generic = 0;

    result->file = 0;
    result->line = -1;

    return result;
}


/*
 * Parsing of variable expansions
 */

typedef struct VAR_PARSE_GROUP
{
    struct dynamic_array elems[ 1 ];
} VAR_PARSE_GROUP;

typedef struct VAR_PARSE_ACTIONS
{
    struct dynamic_array elems[ 1 ];
} VAR_PARSE_ACTIONS;

#define VAR_PARSE_TYPE_VAR      0
#define VAR_PARSE_TYPE_STRING   1
#define VAR_PARSE_TYPE_FILE     2

typedef struct _var_parse
{
    int type;  /* string, variable or file */
} VAR_PARSE;

typedef struct
{
    VAR_PARSE base;
    VAR_PARSE_GROUP * name;
    VAR_PARSE_GROUP * subscript;
    struct dynamic_array modifiers[ 1 ];
} VAR_PARSE_VAR;

typedef struct
{
    VAR_PARSE base;
    OBJECT * s;
} VAR_PARSE_STRING;

typedef struct
{
    VAR_PARSE base;
    struct dynamic_array filename[ 1 ];
    struct dynamic_array contents[ 1 ];
} VAR_PARSE_FILE;

static void var_parse_free( VAR_PARSE * );


/*
 * VAR_PARSE_GROUP
 */

static VAR_PARSE_GROUP * var_parse_group_new()
{
    VAR_PARSE_GROUP * const result = BJAM_MALLOC( sizeof( VAR_PARSE_GROUP ) );
    dynamic_array_init( result->elems );
    return result;
}

static void var_parse_group_free( VAR_PARSE_GROUP * group )
{
    int i;
    for ( i = 0; i < group->elems->size; ++i )
        var_parse_free( dynamic_array_at( VAR_PARSE *, group->elems, i ) );
    dynamic_array_free( group->elems );
    BJAM_FREE( group );
}

static void var_parse_group_add( VAR_PARSE_GROUP * group, VAR_PARSE * elem )
{
    dynamic_array_push( group->elems, elem );
}

static void var_parse_group_maybe_add_constant( VAR_PARSE_GROUP * group,
    char const * start, char const * end )
{
    if ( start != end )
    {
        string buf[ 1 ];
        VAR_PARSE_STRING * const value = (VAR_PARSE_STRING *)BJAM_MALLOC(
            sizeof(VAR_PARSE_STRING) );
        value->base.type = VAR_PARSE_TYPE_STRING;
        string_new( buf );
        string_append_range( buf, start, end );
        value->s = object_new( buf->value );
        string_free( buf );
        var_parse_group_add( group, (VAR_PARSE *)value );
    }
}

VAR_PARSE_STRING * var_parse_group_as_literal( VAR_PARSE_GROUP * group )
{
    if ( group->elems->size == 1  )
    {
        VAR_PARSE * result = dynamic_array_at( VAR_PARSE *, group->elems, 0 );
        if ( result->type == VAR_PARSE_TYPE_STRING )
            return (VAR_PARSE_STRING *)result;
    }
    return 0;
}


/*
 * VAR_PARSE_ACTIONS
 */

static VAR_PARSE_ACTIONS * var_parse_actions_new()
{
    VAR_PARSE_ACTIONS * const result = (VAR_PARSE_ACTIONS *)BJAM_MALLOC(
        sizeof(VAR_PARSE_ACTIONS) );
    dynamic_array_init( result->elems );
    return result;
}

static void var_parse_actions_free( VAR_PARSE_ACTIONS * actions )
{
    int i;
    for ( i = 0; i < actions->elems->size; ++i )
        var_parse_group_free( dynamic_array_at( VAR_PARSE_GROUP *,
            actions->elems, i ) );
    dynamic_array_free( actions->elems );
    BJAM_FREE( actions );
}


/*
 * VAR_PARSE_VAR
 */

static VAR_PARSE_VAR * var_parse_var_new()
{
    VAR_PARSE_VAR * result = BJAM_MALLOC( sizeof( VAR_PARSE_VAR ) );
    result->base.type = VAR_PARSE_TYPE_VAR;
    result->name = var_parse_group_new();
    result->subscript = 0;
    dynamic_array_init( result->modifiers );
    return result;
}

static void var_parse_var_free( VAR_PARSE_VAR * var )
{
    int i;
    var_parse_group_free( var->name );
    if ( var->subscript )
        var_parse_group_free( var->subscript );
    for ( i = 0; i < var->modifiers->size; ++i )
        var_parse_group_free( dynamic_array_at( VAR_PARSE_GROUP *,
            var->modifiers, i ) );
    dynamic_array_free( var->modifiers );
    BJAM_FREE( var );
}

static VAR_PARSE_GROUP * var_parse_var_new_modifier( VAR_PARSE_VAR * var )
{
    VAR_PARSE_GROUP * result = var_parse_group_new();
    dynamic_array_push( var->modifiers, result );
    return result;
}


/*
 * VAR_PARSE_STRING
 */

static void var_parse_string_free( VAR_PARSE_STRING * string )
{
    object_free( string->s );
    BJAM_FREE( string );
}


/*
 * VAR_PARSE_FILE
 */

static VAR_PARSE_FILE * var_parse_file_new( void )
{
    VAR_PARSE_FILE * const result = (VAR_PARSE_FILE *)BJAM_MALLOC( sizeof(
        VAR_PARSE_FILE ) );
    result->base.type = VAR_PARSE_TYPE_FILE;
    dynamic_array_init( result->filename );
    dynamic_array_init( result->contents );
    return result;
}

static void var_parse_file_free( VAR_PARSE_FILE * file )
{
    int i;
    for ( i = 0; i < file->filename->size; ++i )
        var_parse_group_free( dynamic_array_at( VAR_PARSE_GROUP *,
            file->filename, i ) );
    dynamic_array_free( file->filename );
    for ( i = 0; i < file->contents->size; ++i )
        var_parse_group_free( dynamic_array_at( VAR_PARSE_GROUP *,
            file->contents, i ) );
    dynamic_array_free( file->contents );
    BJAM_FREE( file );
}


/*
 * VAR_PARSE
 */

static void var_parse_free( VAR_PARSE * parse )
{
    switch ( parse->type )
    {
        case VAR_PARSE_TYPE_VAR:
            var_parse_var_free( (VAR_PARSE_VAR *)parse );
            break;

        case VAR_PARSE_TYPE_STRING:
            var_parse_string_free( (VAR_PARSE_STRING *)parse );
            break;

        case VAR_PARSE_TYPE_FILE:
            var_parse_file_free( (VAR_PARSE_FILE *)parse );
            break;

        default:
            assert( !"Invalid type" );
    }
}


/*
 * Compile VAR_PARSE
 */

static void var_parse_group_compile( VAR_PARSE_GROUP const * parse,
    compiler * c );

static void var_parse_var_compile( VAR_PARSE_VAR const * parse, compiler * c )
{
    int expand_name = 0;
    int is_get_grist = 0;
    int has_modifiers = 0;
    /* Special case common modifiers */
    if ( parse->modifiers->size == 1 )
    {
        VAR_PARSE_GROUP * mod = dynamic_array_at( VAR_PARSE_GROUP *, parse->modifiers, 0 );
        if ( mod->elems->size == 1 )
        {
            VAR_PARSE * mod1 = dynamic_array_at( VAR_PARSE *, mod->elems, 0 );
            if ( mod1->type == VAR_PARSE_TYPE_STRING )
            {
                OBJECT * s = ( (VAR_PARSE_STRING *)mod1 )->s;
                if ( ! strcmp ( object_str( s ), "G" ) )
                {
                    is_get_grist = 1;
                }
            }
        }
    }
    /* If there are modifiers, emit them in reverse order. */
    if ( parse->modifiers->size > 0 && !is_get_grist )
    {
        int i;
        has_modifiers = 1;
        for ( i = 0; i < parse->modifiers->size; ++i )
            var_parse_group_compile( dynamic_array_at( VAR_PARSE_GROUP *,
                parse->modifiers, parse->modifiers->size - i - 1 ), c );
    }

    /* If there is a subscript, emit it. */
    if ( parse->subscript )
        var_parse_group_compile( parse->subscript, c );

    /* If the variable name is empty, look it up. */
    if ( parse->name->elems->size == 0 )
        compile_emit( c, INSTR_PUSH_VAR, compile_emit_constant( c,
            constant_empty ) );
    /* If the variable name does not need to be expanded, look it up. */
    else if ( parse->name->elems->size == 1 && dynamic_array_at( VAR_PARSE *,
        parse->name->elems, 0 )->type == VAR_PARSE_TYPE_STRING )
    {
        OBJECT * const name = ( (VAR_PARSE_STRING *)dynamic_array_at(
            VAR_PARSE *, parse->name->elems, 0 ) )->s;
        int const idx = get_argument_index( object_str( name ) );
        if ( idx != -1 )
            compile_emit( c, INSTR_PUSH_ARG, idx );
        else
            compile_emit( c, INSTR_PUSH_VAR, compile_emit_constant( c, name ) );
    }
    /* Otherwise, push the var names and use the group instruction. */
    else
    {
        var_parse_group_compile( parse->name, c );
        expand_name = 1;
    }

    /** Select the instruction for expanding the variable. */
    if ( !has_modifiers && !parse->subscript && !expand_name )
        ;
    else if ( !has_modifiers && !parse->subscript && expand_name )
        compile_emit( c, INSTR_PUSH_GROUP, 0 );
    else if ( !has_modifiers && parse->subscript && !expand_name )
        compile_emit( c, INSTR_APPLY_INDEX, 0 );
    else if ( !has_modifiers && parse->subscript && expand_name )
        compile_emit( c, INSTR_APPLY_INDEX_GROUP, 0 );
    else if ( has_modifiers && !parse->subscript && !expand_name )
        compile_emit( c, INSTR_APPLY_MODIFIERS, parse->modifiers->size );
    else if ( has_modifiers && !parse->subscript && expand_name )
        compile_emit( c, INSTR_APPLY_MODIFIERS_GROUP, parse->modifiers->size );
    else if ( has_modifiers && parse->subscript && !expand_name )
        compile_emit( c, INSTR_APPLY_INDEX_MODIFIERS, parse->modifiers->size );
    else if ( has_modifiers && parse->subscript && expand_name )
        compile_emit( c, INSTR_APPLY_INDEX_MODIFIERS_GROUP,
            parse->modifiers->size );

    /* Now apply any special modifiers */
    if ( is_get_grist )
    {
        compile_emit( c, INSTR_GET_GRIST, 0 );
    }
}

static void var_parse_string_compile( VAR_PARSE_STRING const * parse,
    compiler * c )
{
    compile_emit( c, INSTR_PUSH_CONSTANT, compile_emit_constant( c, parse->s )
        );
}

static void var_parse_file_compile( VAR_PARSE_FILE const * parse, compiler * c )
{
    int i;
    for ( i = 0; i < parse->filename->size; ++i )
        var_parse_group_compile( dynamic_array_at( VAR_PARSE_GROUP *,
            parse->filename, parse->filename->size - i - 1 ), c );
    compile_emit( c, INSTR_APPEND_STRINGS, parse->filename->size );
    for ( i = 0; i < parse->contents->size; ++i )
        var_parse_group_compile( dynamic_array_at( VAR_PARSE_GROUP *,
            parse->contents, parse->contents->size - i - 1 ), c );
    compile_emit( c, INSTR_WRITE_FILE, parse->contents->size );
}

static void var_parse_compile( VAR_PARSE const * parse, compiler * c )
{
    switch ( parse->type )
    {
        case VAR_PARSE_TYPE_VAR:
            var_parse_var_compile( (VAR_PARSE_VAR const *)parse, c );
            break;

        case VAR_PARSE_TYPE_STRING:
            var_parse_string_compile( (VAR_PARSE_STRING const *)parse, c );
            break;

        case VAR_PARSE_TYPE_FILE:
            var_parse_file_compile( (VAR_PARSE_FILE const *)parse, c );
            break;

        default:
            assert( !"Unknown var parse type." );
    }
}

static void var_parse_group_compile( VAR_PARSE_GROUP const * parse, compiler * c
    )
{
    /* Emit the elements in reverse order. */
    int i;
    for ( i = 0; i < parse->elems->size; ++i )
        var_parse_compile( dynamic_array_at( VAR_PARSE *, parse->elems,
            parse->elems->size - i - 1 ), c );
    /* If there are no elements, emit an empty string. */
    if ( parse->elems->size == 0 )
        compile_emit( c, INSTR_PUSH_CONSTANT, compile_emit_constant( c,
            constant_empty ) );
    /* If there is more than one element, combine them. */
    if ( parse->elems->size > 1 )
        compile_emit( c, INSTR_COMBINE_STRINGS, parse->elems->size );
}

static void var_parse_actions_compile( VAR_PARSE_ACTIONS const * actions,
    compiler * c )
{
    int i;
    for ( i = 0; i < actions->elems->size; ++i )
        var_parse_group_compile( dynamic_array_at( VAR_PARSE_GROUP *,
            actions->elems, actions->elems->size - i - 1 ), c );
    compile_emit( c, INSTR_OUTPUT_STRINGS, actions->elems->size );
}


/*
 * Parse VAR_PARSE_VAR
 */

static VAR_PARSE * parse_at_file( char const * start, char const * mid,
    char const * end );
static VAR_PARSE * parse_variable( char const * * string );
static int try_parse_variable( char const * * s_, char const * * string,
    VAR_PARSE_GROUP * out );
static void balance_parentheses( char const * * s_, char const * * string,
    VAR_PARSE_GROUP * out );
static void parse_var_string( char const * first, char const * last,
    struct dynamic_array * out );


/*
 * Parses a string that can contain variables to expand.
 */

static VAR_PARSE_GROUP * parse_expansion( char const * * string )
{
    VAR_PARSE_GROUP * result = var_parse_group_new();
    char const * s = *string;
    for ( ; ; )
    {
        if ( try_parse_variable( &s, string, result ) ) {}
        else if ( s[ 0 ] == '\0' )
        {
            var_parse_group_maybe_add_constant( result, *string, s );
            return result;
        }
        else
            ++s;
    }
}

static VAR_PARSE_ACTIONS * parse_actions( char const * string )
{
    VAR_PARSE_ACTIONS * const result = var_parse_actions_new();
    parse_var_string( string, string + strlen( string ), result->elems );
    return result;
}

/*
 * Checks whether the string a *s_ starts with a variable expansion "$(".
 * *string should point to the first unemitted character before *s. If *s_
 * starts with variable expansion, appends elements to out up to the closing
 * ")", and adjusts *s_ and *string to point to next character. Returns 1 if s_
 * starts with a variable, 0 otherwise.
 */

static int try_parse_variable( char const * * s_, char const * * string,
    VAR_PARSE_GROUP * out )
{
    char const * s = *s_;
    if ( s[ 0 ] == '$' && s[ 1 ] == '(' )
    {
        var_parse_group_maybe_add_constant( out, *string, s );
        s += 2;
        var_parse_group_add( out, parse_variable( &s ) );
        *string = s;
        *s_ = s;
        return 1;
    }
    if ( s[ 0 ] == '@' && s[ 1 ] == '(' )
    {
        int depth = 1;
        char const * ine;
        char const * split = 0;
        var_parse_group_maybe_add_constant( out, *string, s );
        s += 2;
        ine = s;

        /* Scan the content of the response file @() section. */
        while ( *ine && ( depth > 0 ) )
        {
            switch ( *ine )
            {
            case '(': ++depth; break;
            case ')': --depth; break;
            case ':':
                if ( ( depth == 1 ) && ( ine[ 1 ] == 'E' ) && ( ine[ 2 ] == '='
                    ) )
                    split = ine;
                break;
            }
            ++ine;
        }

        if ( !split || depth )
            return 0;

        var_parse_group_add( out, parse_at_file( s, split, ine - 1 ) );
        *string = ine;
        *s_ = ine;
        return 1;
    }
    return 0;
}


static char const * current_file = "";
static int current_line;

static void parse_error( char const * message )
{
    printf( "%s:%d: %s\n", current_file, current_line, message );
}


/*
 * Parses a single variable up to the closing ")" and adjusts *string to point
 * to the next character. *string should point to the character immediately
 * after the initial "$(".
 */

static VAR_PARSE * parse_variable( char const * * string )
{
    VAR_PARSE_VAR * const result = var_parse_var_new();
    VAR_PARSE_GROUP * const name = result->name;
    char const * s = *string;
    for ( ; ; )
    {
        if ( try_parse_variable( &s, string, name ) ) {}
        else if ( s[ 0 ] == ':' )
        {
            VAR_PARSE_GROUP * mod;
            var_parse_group_maybe_add_constant( name, *string, s );
            ++s;
            *string = s;
            mod = var_parse_var_new_modifier( result );
            for ( ; ; )
            {
                if ( try_parse_variable( &s, string, mod ) ) {}
                else if ( s[ 0 ] == ')' )
                {
                    var_parse_group_maybe_add_constant( mod, *string, s );
                    *string = ++s;
                    return (VAR_PARSE *)result;
                }
                else if ( s[ 0 ] == '(' )
                {
                    ++s;
                    balance_parentheses( &s, string, mod );
                }
                else if ( s[ 0 ] == ':' )
                {
                    var_parse_group_maybe_add_constant( mod, *string, s );
                    *string = ++s;
                    mod = var_parse_var_new_modifier( result );
                }
                else if ( s[ 0 ] == '[' )
                {
                    parse_error("unexpected subscript");
                    ++s;
                }
                else if ( s[ 0 ] == '\0' )
                {
                    parse_error( "unbalanced parentheses" );
                    var_parse_group_maybe_add_constant( mod, *string, s );
                    *string = s;
                    return (VAR_PARSE *)result;
                }
                else
                    ++s;
            }
        }
        else if ( s[ 0 ] == '[' )
        {
            VAR_PARSE_GROUP * subscript = var_parse_group_new();
            result->subscript = subscript;
            var_parse_group_maybe_add_constant( name, *string, s );
            *string = ++s;
            for ( ; ; )
            {
                if ( try_parse_variable( &s, string, subscript ) ) {}
                else if ( s[ 0 ] == ']' )
                {
                    var_parse_group_maybe_add_constant( subscript, *string, s );
                    *string = ++s;
                    if ( s[ 0 ] != ')' && s[ 0 ] != ':' && s[ 0 ] != '\0' )
                        parse_error( "unexpected text following []" );
                    break;
                }
                else if ( isdigit( s[ 0 ] ) || s[ 0 ] == '-' )
                {
                    ++s;
                }
                else if ( s[ 0 ] == '\0' )
                {
                    parse_error( "malformed subscript" );
                    break;
                }
                else
                {
                    parse_error( "malformed subscript" );
                    ++s;
                }
            }
        }
        else if ( s[ 0 ] == ')' )
        {
            var_parse_group_maybe_add_constant( name, *string, s );
            *string = ++s;
            return (VAR_PARSE *)result;
        }
        else if ( s[ 0 ] == '(' )
        {
            ++s;
            balance_parentheses( &s, string, name );
        }
        else if ( s[ 0 ] == '\0' )
        {
            parse_error( "unbalanced parentheses" );
            var_parse_group_maybe_add_constant( name, *string, s );
            *string = s;
            return (VAR_PARSE *)result;
        }
        else
            ++s;
    }
}

static void parse_var_string( char const * first, char const * last,
    struct dynamic_array * out )
{
    char const * saved = first;
    while ( first != last )
    {
        /* Handle whitespace. */
        while ( first != last && isspace( *first ) ) ++first;
        if ( saved != first )
        {
            VAR_PARSE_GROUP * const group = var_parse_group_new();
            var_parse_group_maybe_add_constant( group, saved, first );
            saved = first;
            dynamic_array_push( out, group );
        }
        if ( first == last ) break;

        /* Handle non-whitespace */
        {
            VAR_PARSE_GROUP * group = var_parse_group_new();
            for ( ; ; )
            {
                if ( first == last || isspace( *first ) )
                {
                    var_parse_group_maybe_add_constant( group, saved, first );
                    saved = first;
                    break;
                }
                if ( try_parse_variable( &first, &saved, group ) )
                    assert( first <= last );
                else
                    ++first;
            }
            dynamic_array_push( out, group );
        }
    }
}

/*
 * start should point to the character immediately following the opening "@(",
 * mid should point to the ":E=", and end should point to the closing ")".
 */

static VAR_PARSE * parse_at_file( char const * start, char const * mid,
    char const * end )
{
    VAR_PARSE_FILE * result = var_parse_file_new();
    parse_var_string( start, mid, result->filename );
    parse_var_string( mid + 3, end, result->contents );
    return (VAR_PARSE *)result;
}

/*
 * Given that *s_ points to the character after a "(", parses up to the matching
 * ")". *string should point to the first unemitted character before *s_.
 *
 * When the function returns, *s_ will point to the character after the ")", and
 * *string will point to the first unemitted character before *s_. The range
 * from *string to *s_ does not contain any variables that need to be expanded.
 */

void balance_parentheses( char const * * s_, char const * * string,
    VAR_PARSE_GROUP * out)
{
    int depth = 1;
    char const * s = *s_;
    for ( ; ; )
    {
        if ( try_parse_variable( &s, string, out ) ) { }
        else if ( s[ 0 ] == ':' || s[ 0 ] == '[' )
        {
            parse_error( "unbalanced parentheses" );
            ++s;
        }
        else if ( s[ 0 ] == '\0' )
        {
            parse_error( "unbalanced parentheses" );
            break;
        }
        else if ( s[ 0 ] == ')' )
        {
            ++s;
            if ( --depth == 0 ) break;
        }
        else if ( s[ 0 ] == '(' )
        {
            ++depth;
            ++s;
        }
        else
            ++s;
    }
    *s_ = s;
}


/*
 * Main compile.
 */

#define RESULT_STACK 0
#define RESULT_RETURN 1
#define RESULT_NONE 2

static void compile_parse( PARSE * parse, compiler * c, int result_location );
static struct arg_list * arg_list_compile( PARSE * parse, int * num_arguments );

static void compile_condition( PARSE * parse, compiler * c, int branch_true, int label )
{
    assert( parse->type == PARSE_EVAL );
    switch ( parse->num )
    {
        case EXPR_EXISTS:
            compile_parse( parse->left, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_NOT_EMPTY, label );
            else
                compile_emit_branch( c, INSTR_JUMP_EMPTY, label );
            break;

        case EXPR_EQUALS:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_EQ, label );
            else
                compile_emit_branch( c, INSTR_JUMP_NE, label );
            break;

        case EXPR_NOTEQ:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_NE, label );
            else
                compile_emit_branch( c, INSTR_JUMP_EQ, label );
            break;

        case EXPR_LESS:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_LT, label );
            else
                compile_emit_branch( c, INSTR_JUMP_GE, label );
            break;

        case EXPR_LESSEQ:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_LE, label );
            else
                compile_emit_branch( c, INSTR_JUMP_GT, label );
            break;

        case EXPR_MORE:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_GT, label );
            else
                compile_emit_branch( c, INSTR_JUMP_LE, label );
            break;

        case EXPR_MOREEQ:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_GE, label );
            else
                compile_emit_branch( c, INSTR_JUMP_LT, label );
            break;

        case EXPR_IN:
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( branch_true )
                compile_emit_branch( c, INSTR_JUMP_IN, label );
            else
                compile_emit_branch( c, INSTR_JUMP_NOT_IN, label );
            break;

        case EXPR_AND:
            if ( branch_true )
            {
                int f = compile_new_label( c );
                compile_condition( parse->left, c, 0, f );
                compile_condition( parse->right, c, 1, label );
                compile_set_label( c, f );
            }
            else
            {
                compile_condition( parse->left, c, 0, label );
                compile_condition( parse->right, c, 0, label );
            }
            break;

        case EXPR_OR:
            if ( branch_true )
            {
                compile_condition( parse->left, c, 1, label );
                compile_condition( parse->right, c, 1, label );
            }
            else
            {
                int t = compile_new_label( c );
                compile_condition( parse->left, c, 1, t );
                compile_condition( parse->right, c, 0, label );
                compile_set_label( c, t );
            }
            break;

        case EXPR_NOT:
            compile_condition( parse->left, c, !branch_true, label );
            break;
    }
}

static void adjust_result( compiler * c, int actual_location,
    int desired_location )
{
    if ( actual_location == desired_location )
        ;
    else if ( actual_location == RESULT_STACK && desired_location == RESULT_RETURN )
        compile_emit( c, INSTR_SET_RESULT, 0 );
    else if ( actual_location == RESULT_STACK && desired_location == RESULT_NONE )
        compile_emit( c, INSTR_POP, 0 );
    else if ( actual_location == RESULT_RETURN && desired_location == RESULT_STACK )
        compile_emit( c, INSTR_PUSH_RESULT, 0 );
    else if ( actual_location == RESULT_RETURN && desired_location == RESULT_NONE )
        ;
    else if ( actual_location == RESULT_NONE && desired_location == RESULT_STACK )
        compile_emit( c, INSTR_PUSH_EMPTY, 0 );
    else if ( actual_location == RESULT_NONE && desired_location == RESULT_RETURN )
    {
        compile_emit( c, INSTR_PUSH_EMPTY, 0 );
        compile_emit( c, INSTR_SET_RESULT, 0 );
    }
    else
        assert( !"invalid result location" );
}

static char const * parse_type( PARSE * parse )
{
    switch ( parse->type )
    {
        case PARSE_APPEND: return "append";
        case PARSE_EVAL: return "eval";
        case PARSE_RULES: return "rules";
        default: return "unknown";
    }
}

static void compile_append_chain( PARSE * parse, compiler * c )
{
    assert( parse->type == PARSE_APPEND );
    if ( parse->left->type == PARSE_NULL )
        compile_parse( parse->right, c, RESULT_STACK );
    else
    {
        if ( parse->left->type == PARSE_APPEND )
            compile_append_chain( parse->left, c );
        else
            compile_parse( parse->left, c, RESULT_STACK );
        compile_parse( parse->right, c, RESULT_STACK );
        compile_emit( c, INSTR_PUSH_APPEND, 0 );
    }
}

static void compile_parse( PARSE * parse, compiler * c, int result_location )
{
    if ( parse->type == PARSE_APPEND )
    {
        compile_append_chain( parse, c );
        adjust_result( c, RESULT_STACK, result_location );
    }
    else if ( parse->type == PARSE_EVAL )
    {
        /* FIXME: This is only needed because of the bizarre parsing of
         * conditions.
         */
        if ( parse->num == EXPR_EXISTS )
            compile_parse( parse->left, c, result_location );
        else
        {
            int f = compile_new_label( c );
            int end = compile_new_label( c );

            printf( "%s:%d: Conditional used as list (check operator "
                "precedence).\n", object_str( parse->file ), parse->line );

            /* Emit the condition */
            compile_condition( parse, c, 0, f );
            compile_emit( c, INSTR_PUSH_CONSTANT, compile_emit_constant( c,
                constant_true ) );
            compile_emit_branch( c, INSTR_JUMP, end );
            compile_set_label( c, f );
            compile_emit( c, INSTR_PUSH_EMPTY, 0 );
            compile_set_label( c, end );
            adjust_result( c, RESULT_STACK, result_location );
        }
    }
    else if ( parse->type == PARSE_FOREACH )
    {
        int var = compile_emit_constant( c, parse->string );
        int top = compile_new_label( c );
        int end = compile_new_label( c );

        /*
         * Evaluate the list.
         */
        compile_parse( parse->left, c, RESULT_STACK );

        /* Localize the loop variable */
        if ( parse->num )
        {
            compile_emit( c, INSTR_PUSH_EMPTY, 0 );
            compile_emit( c, INSTR_PUSH_LOCAL, var );
            compile_emit( c, INSTR_SWAP, 1 );
        }

        compile_emit( c, INSTR_FOR_INIT, 0 );
        compile_set_label( c, top );
        compile_emit_branch( c, INSTR_FOR_LOOP, end );
        compile_emit( c, INSTR_SET, var );

        /* Run the loop body */
        compile_parse( parse->right, c, RESULT_NONE );

        compile_emit_branch( c, INSTR_JUMP, top );
        compile_set_label( c, end );

        if ( parse->num )
            compile_emit( c, INSTR_POP_LOCAL, var );

        adjust_result( c, RESULT_NONE, result_location);
    }
    else if ( parse->type == PARSE_IF )
    {
        int f = compile_new_label( c );
        /* Emit the condition */
        compile_condition( parse->left, c, 0, f );
        /* Emit the if block */
        compile_parse( parse->right, c, result_location );
        if ( parse->third->type != PARSE_NULL || result_location != RESULT_NONE )
        {
            /* Emit the else block */
            int end = compile_new_label( c );
            compile_emit_branch( c, INSTR_JUMP, end );
            compile_set_label( c, f );
            compile_parse( parse->third, c, result_location );
            compile_set_label( c, end );
        }
        else
            compile_set_label( c, f );

    }
    else if ( parse->type == PARSE_WHILE )
    {
        int nested_result = result_location == RESULT_NONE
            ? RESULT_NONE
            : RESULT_RETURN;
        int test = compile_new_label( c );
        int top = compile_new_label( c );
        /* Make sure that we return an empty list if the loop runs zero times.
         */
        adjust_result( c, RESULT_NONE, nested_result );
        /* Jump to the loop test. */
        compile_emit_branch( c, INSTR_JUMP, test );
        compile_set_label( c, top );
        /* Emit the loop body. */
        compile_parse( parse->right, c, nested_result );
        /* Emit the condition. */
        compile_set_label( c, test );
        compile_condition( parse->left, c, 1, top );

        adjust_result( c, nested_result, result_location );
    }
    else if ( parse->type == PARSE_INCLUDE )
    {
        compile_parse( parse->left, c, RESULT_STACK );
        compile_emit( c, INSTR_INCLUDE, 0 );
        compile_emit( c, INSTR_BIND_MODULE_VARIABLES, 0 );
        adjust_result( c, RESULT_NONE, result_location );
    }
    else if ( parse->type == PARSE_MODULE )
    {
        int const nested_result = result_location == RESULT_NONE
            ? RESULT_NONE
            : RESULT_RETURN;
        compile_parse( parse->left, c, RESULT_STACK );
        compile_emit( c, INSTR_PUSH_MODULE, 0 );
        compile_parse( parse->right, c, nested_result );
        compile_emit( c, INSTR_POP_MODULE, 0 );
        adjust_result( c, nested_result, result_location );
    }
    else if ( parse->type == PARSE_CLASS )
    {
        /* Evaluate the class name. */
        compile_parse( parse->left->right, c, RESULT_STACK );
        /* Evaluate the base classes. */
        if ( parse->left->left )
            compile_parse( parse->left->left->right, c, RESULT_STACK );
        else
            compile_emit( c, INSTR_PUSH_EMPTY, 0 );
        compile_emit( c, INSTR_CLASS, 0 );
        compile_parse( parse->right, c, RESULT_NONE );
        compile_emit( c, INSTR_BIND_MODULE_VARIABLES, 0 );
        compile_emit( c, INSTR_POP_MODULE, 0 );

        adjust_result( c, RESULT_NONE, result_location );
    }
    else if ( parse->type == PARSE_LIST )
    {
        OBJECT * const o = parse->string;
        char const * s = object_str( o );
        VAR_PARSE_GROUP * group;
        current_file = object_str( parse->file );
        current_line = parse->line;
        group = parse_expansion( &s );
        var_parse_group_compile( group, c );
        var_parse_group_free( group );
        adjust_result( c, RESULT_STACK, result_location );
    }
    else if ( parse->type == PARSE_LOCAL )
    {
        int nested_result = result_location == RESULT_NONE
            ? RESULT_NONE
            : RESULT_RETURN;
        /* This should be left recursive group of compile_appends. */
        PARSE * vars = parse->left;

        /* Special case an empty list of vars */
        if ( vars->type == PARSE_NULL )
        {
            compile_parse( parse->right, c, RESULT_NONE );
            compile_parse( parse->third, c, result_location );
            nested_result = result_location;
        }
        /* Check whether there is exactly one variable with a constant name. */
        else if ( vars->left->type == PARSE_NULL &&
            vars->right->type == PARSE_LIST )
        {
            char const * s = object_str( vars->right->string );
            VAR_PARSE_GROUP * group;
            current_file = object_str( parse->file );
            current_line = parse->line;
            group = parse_expansion( &s );
            if ( group->elems->size == 1 && dynamic_array_at( VAR_PARSE *,
                group->elems, 0 )->type == VAR_PARSE_TYPE_STRING )
            {
                int const name = compile_emit_constant( c, (
                    (VAR_PARSE_STRING *)dynamic_array_at( VAR_PARSE *,
                    group->elems, 0 ) )->s );
                var_parse_group_free( group );
                compile_parse( parse->right, c, RESULT_STACK );
                compile_emit( c, INSTR_PUSH_LOCAL, name );
                compile_parse( parse->third, c, nested_result );
                compile_emit( c, INSTR_POP_LOCAL, name );
            }
            else
            {
                var_parse_group_compile( group, c );
                var_parse_group_free( group );
                compile_parse( parse->right, c, RESULT_STACK );
                compile_emit( c, INSTR_PUSH_LOCAL_GROUP, 0 );
                compile_parse( parse->third, c, nested_result );
                compile_emit( c, INSTR_POP_LOCAL_GROUP, 0 );
            }
        }
        else
        {
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            compile_emit( c, INSTR_PUSH_LOCAL_GROUP, 0 );
            compile_parse( parse->third, c, nested_result );
            compile_emit( c, INSTR_POP_LOCAL_GROUP, 0 );
        }
        adjust_result( c, nested_result, result_location );
    }
    else if ( parse->type == PARSE_ON )
    {
        if ( parse->right->type == PARSE_APPEND &&
            parse->right->left->type == PARSE_NULL &&
            parse->right->right->type == PARSE_LIST )
        {
            /* [ on $(target) return $(variable) ] */
            PARSE * value = parse->right->right;
            OBJECT * const o = value->string;
            char const * s = object_str( o );
            VAR_PARSE_GROUP * group;
            OBJECT * varname = 0;
            current_file = object_str( value->file );
            current_line = value->line;
            group = parse_expansion( &s );
            if ( group->elems->size == 1 )
            {
                VAR_PARSE * one = dynamic_array_at( VAR_PARSE *, group->elems, 0 );
                if ( one->type == VAR_PARSE_TYPE_VAR )
                {
                    VAR_PARSE_VAR * var = ( VAR_PARSE_VAR * )one;
                    if ( var->modifiers->size == 0 && !var->subscript && var->name->elems->size == 1 )
                    {
                        VAR_PARSE * name = dynamic_array_at( VAR_PARSE *, var->name->elems, 0 );
                        if ( name->type == VAR_PARSE_TYPE_STRING )
                        {
                            varname = ( ( VAR_PARSE_STRING * )name )->s;
                        }
                    }
                }
            }
            if ( varname )
            {
                /* We have one variable with a fixed name and no modifiers. */
                compile_parse( parse->left, c, RESULT_STACK );
                compile_emit( c, INSTR_GET_ON, compile_emit_constant( c, varname ) );
            }
            else
            {
                /* Too complex.  Fall back on push/pop. */
                int end = compile_new_label( c );
                compile_parse( parse->left, c, RESULT_STACK );
                compile_emit_branch( c, INSTR_PUSH_ON, end );
                var_parse_group_compile( group, c );
                compile_emit( c, INSTR_POP_ON, 0 );
                compile_set_label( c, end );
            }
            var_parse_group_free( group );
        }
        else
        {
            int end = compile_new_label( c );
            compile_parse( parse->left, c, RESULT_STACK );
            compile_emit_branch( c, INSTR_PUSH_ON, end );
            compile_parse( parse->right, c, RESULT_STACK );
            compile_emit( c, INSTR_POP_ON, 0 );
            compile_set_label( c, end );
        }
        adjust_result( c, RESULT_STACK, result_location );
    }
    else if ( parse->type == PARSE_RULE )
    {
        PARSE * p;
        int n = 0;
        VAR_PARSE_GROUP * group;
        char const * s = object_str( parse->string );

        if ( parse->left->left || parse->left->right->type != PARSE_NULL )
            for ( p = parse->left; p; p = p->left )
            {
                compile_parse( p->right, c, RESULT_STACK );
                ++n;
            }

        current_file = object_str( parse->file );
        current_line = parse->line;
        group = parse_expansion( &s );

        if ( group->elems->size == 2 &&
            dynamic_array_at( VAR_PARSE *, group->elems, 0 )->type == VAR_PARSE_TYPE_VAR &&
            dynamic_array_at( VAR_PARSE *, group->elems, 1 )->type == VAR_PARSE_TYPE_STRING &&
            ( object_str( ( (VAR_PARSE_STRING *)dynamic_array_at( VAR_PARSE *, group->elems, 1 ) )->s )[ 0 ] == '.' ) )
        {
            VAR_PARSE_STRING * access = (VAR_PARSE_STRING *)dynamic_array_at( VAR_PARSE *, group->elems, 1 );
            OBJECT * member = object_new( object_str( access->s ) + 1 );
            /* Emit the object */
            var_parse_var_compile( (VAR_PARSE_VAR *)dynamic_array_at( VAR_PARSE *, group->elems, 0 ), c );
            var_parse_group_free( group );
            compile_emit( c, INSTR_CALL_MEMBER_RULE, n );
            compile_emit( c, compile_emit_constant( c, member ), parse->line );
            object_free( member );
        }
        else
        {
            var_parse_group_compile( group, c );
            var_parse_group_free( group );
            compile_emit( c, INSTR_CALL_RULE, n );
            compile_emit( c, compile_emit_constant( c, parse->string ), parse->line );
        }

        adjust_result( c, RESULT_STACK, result_location );
    }
    else if ( parse->type == PARSE_RULES )
    {
        do compile_parse( parse->left, c, RESULT_NONE );
        while ( ( parse = parse->right )->type == PARSE_RULES );
        compile_parse( parse, c, result_location );
    }
    else if ( parse->type == PARSE_SET )
    {
        PARSE * vars = parse->left;
        unsigned int op_code;
        unsigned int op_code_group;

        switch ( parse->num )
        {
        case ASSIGN_APPEND: op_code = INSTR_APPEND; op_code_group = INSTR_APPEND_GROUP; break;
        case ASSIGN_DEFAULT: op_code = INSTR_DEFAULT; op_code_group = INSTR_DEFAULT_GROUP; break;
        default: op_code = INSTR_SET; op_code_group = INSTR_SET_GROUP; break;
        }

        /* Check whether there is exactly one variable with a constant name. */
        if ( vars->type == PARSE_LIST )
        {
            char const * s = object_str( vars->string );
            VAR_PARSE_GROUP * group;
            current_file = object_str( parse->file );
            current_line = parse->line;
            group = parse_expansion( &s );
            if ( group->elems->size == 1 && dynamic_array_at( VAR_PARSE *,
                group->elems, 0 )->type == VAR_PARSE_TYPE_STRING )
            {
                int const name = compile_emit_constant( c, (
                    (VAR_PARSE_STRING *)dynamic_array_at( VAR_PARSE *,
                    group->elems, 0 ) )->s );
                var_parse_group_free( group );
                compile_parse( parse->right, c, RESULT_STACK );
                if ( result_location != RESULT_NONE )
                {
                    compile_emit( c, INSTR_SET_RESULT, 1 );
                }
                compile_emit( c, op_code, name );
            }
            else
            {
                var_parse_group_compile( group, c );
                var_parse_group_free( group );
                compile_parse( parse->right, c, RESULT_STACK );
                if ( result_location != RESULT_NONE )
                {
                    compile_emit( c, INSTR_SET_RESULT, 1 );
                }
                compile_emit( c, op_code_group, 0 );
            }
        }
        else
        {
            compile_parse( parse->left, c, RESULT_STACK );
            compile_parse( parse->right, c, RESULT_STACK );
            if ( result_location != RESULT_NONE )
            {
                compile_emit( c, INSTR_SET_RESULT, 1 );
            }
            compile_emit( c, op_code_group, 0 );
        }
        if ( result_location != RESULT_NONE )
        {
            adjust_result( c, RESULT_RETURN, result_location );
        }
    }
    else if ( parse->type == PARSE_SETCOMP )
    {
        int n_args;
        struct arg_list * args = arg_list_compile( parse->right, &n_args );
        int const rule_id = compile_emit_rule( c, parse->string, parse->left,
            n_args, args, parse->num );
        compile_emit( c, INSTR_RULE, rule_id );
        adjust_result( c, RESULT_NONE, result_location );
    }
    else if ( parse->type == PARSE_SETEXEC )
    {
        int const actions_id = compile_emit_actions( c, parse );
        compile_parse( parse->left, c, RESULT_STACK );
        compile_emit( c, INSTR_ACTIONS, actions_id );
        adjust_result( c, RESULT_NONE, result_location );
    }
    else if ( parse->type == PARSE_SETTINGS )
    {
        compile_parse( parse->left, c, RESULT_STACK );
        compile_parse( parse->third, c, RESULT_STACK );
        compile_parse( parse->right, c, RESULT_STACK );

        switch ( parse->num )
        {
            case ASSIGN_APPEND: compile_emit( c, INSTR_APPEND_ON, 0 ); break;
            case ASSIGN_DEFAULT: compile_emit( c, INSTR_DEFAULT_ON, 0 ); break;
            default: compile_emit( c, INSTR_SET_ON, 0 ); break;
        }

        adjust_result( c, RESULT_STACK, result_location );
    }
    else if ( parse->type == PARSE_SWITCH )
    {
        int const switch_end = compile_new_label( c );
        compile_parse( parse->left, c, RESULT_STACK );

        for ( parse = parse->right; parse; parse = parse->right )
        {
            int const id = compile_emit_constant( c, parse->left->string );
            int const next_case = compile_new_label( c );
            compile_emit( c, INSTR_PUSH_CONSTANT, id );
            compile_emit_branch( c, INSTR_JUMP_NOT_GLOB, next_case );
            compile_parse( parse->left->left, c, result_location );
            compile_emit_branch( c, INSTR_JUMP, switch_end );
            compile_set_label( c, next_case );
        }
        compile_emit( c, INSTR_POP, 0 );
        adjust_result( c, RESULT_NONE, result_location );
        compile_set_label( c, switch_end );
    }
    else if ( parse->type == PARSE_NULL )
        adjust_result( c, RESULT_NONE, result_location );
    else
        assert( !"unknown PARSE type." );
}

OBJECT * function_rulename( FUNCTION * function )
{
    return function->rulename;
}

void function_set_rulename( FUNCTION * function, OBJECT * rulename )
{
    function->rulename = rulename;
}

void function_location( FUNCTION * function_, OBJECT * * file, int * line )
{
    if ( function_->type == FUNCTION_BUILTIN )
    {
        *file = constant_builtin;
        *line = -1;
    }
#ifdef HAVE_PYTHON
    if ( function_->type == FUNCTION_PYTHON )
    {
        *file = constant_builtin;
        *line = -1;
    }
#endif
    else
    {
        JAM_FUNCTION * function = (JAM_FUNCTION *)function_;
        assert( function_->type == FUNCTION_JAM );
        *file = function->file;
        *line = function->line;
    }
}

static struct arg_list * arg_list_compile_builtin( char const * * args,
    int * num_arguments );

FUNCTION * function_builtin( LIST * ( * func )( FRAME * frame, int flags ),
    int flags, char const * * args )
{
    BUILTIN_FUNCTION * result = BJAM_MALLOC( sizeof( BUILTIN_FUNCTION ) );
    result->base.type = FUNCTION_BUILTIN;
    result->base.reference_count = 1;
    result->base.rulename = 0;
    result->base.formal_arguments = arg_list_compile_builtin( args,
        &result->base.num_formal_arguments );
    result->func = func;
    result->flags = flags;
    return (FUNCTION *)result;
}

FUNCTION * function_compile( PARSE * parse )
{
    compiler c[ 1 ];
    JAM_FUNCTION * result;
    compiler_init( c );
    compile_parse( parse, c, RESULT_RETURN );
    compile_emit( c, INSTR_RETURN, 0 );
    result = compile_to_function( c );
    compiler_free( c );
    result->file = object_copy( parse->file );
    result->line = parse->line;
    return (FUNCTION *)result;
}

FUNCTION * function_compile_actions( char const * actions, OBJECT * file,
    int line )
{
    compiler c[ 1 ];
    JAM_FUNCTION * result;
    VAR_PARSE_ACTIONS * parse;
    current_file = object_str( file );
    current_line = line;
    parse = parse_actions( actions );
    compiler_init( c );
    var_parse_actions_compile( parse, c );
    var_parse_actions_free( parse );
    compile_emit( c, INSTR_RETURN, 0 );
    result = compile_to_function( c );
    compiler_free( c );
    result->file = object_copy( file );
    result->line = line;
    return (FUNCTION *)result;
}

static void argument_list_print( struct arg_list * args, int num_args );


/* Define delimiters for type check elements in argument lists (and return type
 * specifications, eventually).
 */
# define TYPE_OPEN_DELIM '['
# define TYPE_CLOSE_DELIM ']'

/*
 * is_type_name() - true iff the given string represents a type check
 * specification.
 */

int is_type_name( char const * s )
{
    return s[ 0 ] == TYPE_OPEN_DELIM && s[ strlen( s ) - 1 ] ==
        TYPE_CLOSE_DELIM;
}

static void argument_error( char const * message, FUNCTION * procedure,
    FRAME * frame, OBJECT * arg )
{
    extern void print_source_line( FRAME * );
    LOL * actual = frame->args;
    backtrace_line( frame->prev );
    printf( "*** argument error\n* rule %s ( ", frame->rulename );
    argument_list_print( procedure->formal_arguments,
        procedure->num_formal_arguments );
    printf( " )\n* called with: ( " );
    lol_print( actual );
    printf( " )\n* %s %s\n", message, arg ? object_str ( arg ) : "" );
    function_location( procedure, &frame->file, &frame->line );
    print_source_line( frame );
    printf( "see definition of rule '%s' being called\n", frame->rulename );
    backtrace( frame->prev );
    exit( 1 );
}

static void type_check_range( OBJECT * type_name, LISTITER iter, LISTITER end,
    FRAME * caller, FUNCTION * called, OBJECT * arg_name )
{
    static module_t * typecheck = 0;

    /* If nothing to check, bail now. */
    if ( iter == end || !type_name )
        return;

    if ( !typecheck )
        typecheck = bindmodule( constant_typecheck );

    /* If the checking rule can not be found, also bail. */
    if ( !typecheck->rules || !hash_find( typecheck->rules, type_name ) )
        return;

    for ( ; iter != end; iter = list_next( iter ) )
    {
        LIST * error;
        FRAME frame[ 1 ];
        frame_init( frame );
        frame->module = typecheck;
        frame->prev = caller;
        frame->prev_user = caller->module->user_module
            ? caller
            : caller->prev_user;

        /* Prepare the argument list */
        lol_add( frame->args, list_new( object_copy( list_item( iter ) ) ) );
        error = evaluate_rule( bindrule( type_name, frame->module ), type_name, frame );

        if ( !list_empty( error ) )
            argument_error( object_str( list_front( error ) ), called, caller,
                arg_name );

        frame_free( frame );
    }
}

static void type_check( OBJECT * type_name, LIST * values, FRAME * caller,
    FUNCTION * called, OBJECT * arg_name )
{
    type_check_range( type_name, list_begin( values ), list_end( values ),
        caller, called, arg_name );
}

void argument_list_check( struct arg_list * formal, int formal_count,
    FUNCTION * function, FRAME * frame )
{
    LOL * all_actual = frame->args;
    int i;

    for ( i = 0; i < formal_count; ++i )
    {
        LIST * actual = lol_get( all_actual, i );
        LISTITER actual_iter = list_begin( actual );
        LISTITER const actual_end = list_end( actual );
        int j;
        for ( j = 0; j < formal[ i ].size; ++j )
        {
            struct argument * formal_arg = &formal[ i ].args[ j ];
            LIST * value;

            switch ( formal_arg->flags )
            {
            case ARG_ONE:
                if ( actual_iter == actual_end )
                    argument_error( "missing argument", function, frame,
                        formal_arg->arg_name );
                type_check_range( formal_arg->type_name, actual_iter,
                    list_next( actual_iter ), frame, function,
                    formal_arg->arg_name );
                actual_iter = list_next( actual_iter );
                break;
            case ARG_OPTIONAL:
                if ( actual_iter == actual_end )
                    value = L0;
                else
                {
                    type_check_range( formal_arg->type_name, actual_iter,
                        list_next( actual_iter ), frame, function,
                        formal_arg->arg_name );
                    actual_iter = list_next( actual_iter );
                }
                break;
            case ARG_PLUS:
                if ( actual_iter == actual_end )
                    argument_error( "missing argument", function, frame,
                        formal_arg->arg_name );
                /* fallthrough */
            case ARG_STAR:
                type_check_range( formal_arg->type_name, actual_iter,
                    actual_end, frame, function, formal_arg->arg_name );
                actual_iter = actual_end;
                break;
            case ARG_VARIADIC:
                return;
            }
        }

        if ( actual_iter != actual_end )
            argument_error( "extra argument", function, frame, list_item(
                actual_iter ) );
    }

    for ( ; i < all_actual->count; ++i )
    {
        LIST * actual = lol_get( all_actual, i );
        if ( !list_empty( actual ) )
            argument_error( "extra argument", function, frame, list_front(
                actual ) );
    }
}

void argument_list_push( struct arg_list * formal, int formal_count,
    FUNCTION * function, FRAME * frame, STACK * s )
{
    LOL * all_actual = frame->args;
    int i;

    for ( i = 0; i < formal_count; ++i )
    {
        LIST * actual = lol_get( all_actual, i );
        LISTITER actual_iter = list_begin( actual );
        LISTITER const actual_end = list_end( actual );
        int j;
        for ( j = 0; j < formal[ i ].size; ++j )
        {
            struct argument * formal_arg = &formal[ i ].args[ j ];
            LIST * value;

            switch ( formal_arg->flags )
            {
            case ARG_ONE:
                if ( actual_iter == actual_end )
                    argument_error( "missing argument", function, frame,
                        formal_arg->arg_name );
                value = list_new( object_copy( list_item( actual_iter ) ) );
                actual_iter = list_next( actual_iter );
                break;
            case ARG_OPTIONAL:
                if ( actual_iter == actual_end )
                    value = L0;
                else
                {
                    value = list_new( object_copy( list_item( actual_iter ) ) );
                    actual_iter = list_next( actual_iter );
                }
                break;
            case ARG_PLUS:
                if ( actual_iter == actual_end )
                    argument_error( "missing argument", function, frame,
                        formal_arg->arg_name );
                /* fallthrough */
            case ARG_STAR:
                value = list_copy_range( actual, actual_iter, actual_end );
                actual_iter = actual_end;
                break;
            case ARG_VARIADIC:
                return;
            }

            type_check( formal_arg->type_name, value, frame, function,
                formal_arg->arg_name );

            if ( formal_arg->index != -1 )
            {
                LIST * * const old = &frame->module->fixed_variables[
                    formal_arg->index ];
                stack_push( s, *old );
                *old = value;
            }
            else
                stack_push( s, var_swap( frame->module, formal_arg->arg_name,
                    value ) );
        }

        if ( actual_iter != actual_end )
            argument_error( "extra argument", function, frame, list_item(
                actual_iter ) );
    }

    for ( ; i < all_actual->count; ++i )
    {
        LIST * const actual = lol_get( all_actual, i );
        if ( !list_empty( actual ) )
            argument_error( "extra argument", function, frame, list_front(
                actual ) );
    }
}

void argument_list_pop( struct arg_list * formal, int formal_count,
    FRAME * frame, STACK * s )
{
    int i;
    for ( i = formal_count - 1; i >= 0; --i )
    {
        int j;
        for ( j = formal[ i ].size - 1; j >= 0 ; --j )
        {
            struct argument * formal_arg = &formal[ i ].args[ j ];

            if ( formal_arg->flags == ARG_VARIADIC )
                continue;
            if ( formal_arg->index != -1 )
            {
                LIST * const old = stack_pop( s );
                LIST * * const pos = &frame->module->fixed_variables[
                    formal_arg->index ];
                list_free( *pos );
                *pos = old;
            }
            else
                var_set( frame->module, formal_arg->arg_name, stack_pop( s ),
                    VAR_SET );
        }
    }
}


struct argument_compiler
{
    struct dynamic_array args[ 1 ];
    struct argument arg;
    int state;
#define ARGUMENT_COMPILER_START         0
#define ARGUMENT_COMPILER_FOUND_TYPE    1
#define ARGUMENT_COMPILER_FOUND_OBJECT  2
#define ARGUMENT_COMPILER_DONE          3
};


static void argument_compiler_init( struct argument_compiler * c )
{
    dynamic_array_init( c->args );
    c->state = ARGUMENT_COMPILER_START;
}

static void argument_compiler_free( struct argument_compiler * c )
{
    dynamic_array_free( c->args );
}

static void argument_compiler_add( struct argument_compiler * c, OBJECT * arg,
    OBJECT * file, int line )
{
    switch ( c->state )
    {
    case ARGUMENT_COMPILER_FOUND_OBJECT:

        if ( object_equal( arg, constant_question_mark ) )
        {
            c->arg.flags = ARG_OPTIONAL;
        }
        else if ( object_equal( arg, constant_plus ) )
        {
            c->arg.flags = ARG_PLUS;
        }
        else if ( object_equal( arg, constant_star ) )
        {
            c->arg.flags = ARG_STAR;
        }

        dynamic_array_push( c->args, c->arg );
        c->state = ARGUMENT_COMPILER_START;

        if ( c->arg.flags != ARG_ONE )
            break;
        /* fall-through */

    case ARGUMENT_COMPILER_START:

        c->arg.type_name = 0;
        c->arg.index = -1;
        c->arg.flags = ARG_ONE;

        if ( is_type_name( object_str( arg ) ) )
        {
            c->arg.type_name = object_copy( arg );
            c->state = ARGUMENT_COMPILER_FOUND_TYPE;
            break;
        }
        /* fall-through */

    case ARGUMENT_COMPILER_FOUND_TYPE:

        if ( is_type_name( object_str( arg ) ) )
        {
            printf( "%s:%d: missing argument name before type name: %s\n",
                object_str( file ), line, object_str( arg ) );
            exit( 1 );
        }

        c->arg.arg_name = object_copy( arg );
        if ( object_equal( arg, constant_star ) )
        {
            c->arg.flags = ARG_VARIADIC;
            dynamic_array_push( c->args, c->arg );
            c->state = ARGUMENT_COMPILER_DONE;
        }
        else
        {
            c->state = ARGUMENT_COMPILER_FOUND_OBJECT;
        }
        break;

    case ARGUMENT_COMPILER_DONE:
        break;
    }
}

static void argument_compiler_recurse( struct argument_compiler * c,
    PARSE * parse )
{
    if ( parse->type == PARSE_APPEND )
    {
        argument_compiler_recurse( c, parse->left );
        argument_compiler_recurse( c, parse->right );
    }
    else if ( parse->type != PARSE_NULL )
    {
        assert( parse->type == PARSE_LIST );
        argument_compiler_add( c, parse->string, parse->file, parse->line );
    }
}

static struct arg_list arg_compile_impl( struct argument_compiler * c,
    OBJECT * file, int line )
{
    struct arg_list result;
    switch ( c->state )
    {
    case ARGUMENT_COMPILER_START:
    case ARGUMENT_COMPILER_DONE:
        break;
    case ARGUMENT_COMPILER_FOUND_TYPE:
        printf( "%s:%d: missing argument name after type name: %s\n",
            object_str( file ), line, object_str( c->arg.type_name ) );
        exit( 1 );
    case ARGUMENT_COMPILER_FOUND_OBJECT:
        dynamic_array_push( c->args, c->arg );
        break;
    }
    result.size = c->args->size;
    result.args = BJAM_MALLOC( c->args->size * sizeof( struct argument ) );
    memcpy( result.args, c->args->data, c->args->size * sizeof( struct argument
        ) );
    return result;
}

static struct arg_list arg_compile( PARSE * parse )
{
    struct argument_compiler c[ 1 ];
    struct arg_list result;
    argument_compiler_init( c );
    argument_compiler_recurse( c, parse );
    result = arg_compile_impl( c, parse->file, parse->line );
    argument_compiler_free( c );
    return result;
}

struct argument_list_compiler
{
    struct dynamic_array args[ 1 ];
};

static void argument_list_compiler_init( struct argument_list_compiler * c )
{
    dynamic_array_init( c->args );
}

static void argument_list_compiler_free( struct argument_list_compiler * c )
{
    dynamic_array_free( c->args );
}

static void argument_list_compiler_add( struct argument_list_compiler * c,
    PARSE * parse )
{
    struct arg_list args = arg_compile( parse );
    dynamic_array_push( c->args, args );
}

static void argument_list_compiler_recurse( struct argument_list_compiler * c,
    PARSE * parse )
{
    if ( parse )
    {
        argument_list_compiler_add( c, parse->right );
        argument_list_compiler_recurse( c, parse->left );
    }
}

static struct arg_list * arg_list_compile( PARSE * parse, int * num_arguments )
{
    if ( parse )
    {
        struct argument_list_compiler c[ 1 ];
        struct arg_list * result;
        argument_list_compiler_init( c );
        argument_list_compiler_recurse( c, parse );
        *num_arguments = c->args->size;
        result = BJAM_MALLOC( c->args->size * sizeof( struct arg_list ) );
        memcpy( result, c->args->data, c->args->size * sizeof( struct arg_list )
            );
        argument_list_compiler_free( c );
        return result;
    }
    *num_arguments = 0;
    return 0;
}

static struct arg_list * arg_list_compile_builtin( char const * * args,
    int * num_arguments )
{
    if ( args )
    {
        struct argument_list_compiler c[ 1 ];
        struct arg_list * result;
        argument_list_compiler_init( c );
        while ( *args )
        {
            struct argument_compiler arg_comp[ 1 ];
            struct arg_list arg;
            argument_compiler_init( arg_comp );
            for ( ; *args; ++args )
            {
                OBJECT * token;
                if ( strcmp( *args, ":" ) == 0 )
                {
                    ++args;
                    break;
                }
                token = object_new( *args );
                argument_compiler_add( arg_comp, token, constant_builtin, -1 );
                object_free( token );
            }
            arg = arg_compile_impl( arg_comp, constant_builtin, -1 );
            dynamic_array_push( c->args, arg );
            argument_compiler_free( arg_comp );
        }
        *num_arguments = c->args->size;
        result = BJAM_MALLOC( c->args->size * sizeof( struct arg_list ) );
        memcpy( result, c->args->data, c->args->size * sizeof( struct arg_list )
            );
        argument_list_compiler_free( c );
        return result;
    }
    *num_arguments = 0;
    return 0;
}

static void argument_list_print( struct arg_list * args, int num_args )
{
    if ( args )
    {
        int i;
        for ( i = 0; i < num_args; ++i )
        {
            int j;
            if ( i ) printf( " : " );
            for ( j = 0; j < args[ i ].size; ++j )
            {
                struct argument * formal_arg = &args[ i ].args[ j ];
                if ( j ) printf( " " );
                if ( formal_arg->type_name )
                    printf( "%s ", object_str( formal_arg->type_name ) );
                printf( "%s", object_str( formal_arg->arg_name ) );
                switch ( formal_arg->flags )
                {
                case ARG_OPTIONAL: printf( " ?" ); break;
                case ARG_PLUS:     printf( " +" ); break;
                case ARG_STAR:     printf( " *" ); break;
                }
            }
        }
    }
}


struct arg_list * argument_list_bind_variables( struct arg_list * formal,
    int formal_count, module_t * module, int * counter )
{
    if ( formal )
    {
        struct arg_list * result = (struct arg_list *)BJAM_MALLOC( sizeof(
            struct arg_list ) * formal_count );
        int i;

        for ( i = 0; i < formal_count; ++i )
        {
            int j;
            struct argument * args = (struct argument *)BJAM_MALLOC( sizeof(
                struct argument ) * formal[ i ].size );
            for ( j = 0; j < formal[ i ].size; ++j )
            {
                args[ j ] = formal[ i ].args[ j ];
                if ( args[ j ].type_name )
                    args[ j ].type_name = object_copy( args[ j ].type_name );
                args[ j ].arg_name = object_copy( args[ j ].arg_name );
                if ( args[ j ].flags != ARG_VARIADIC )
                    args[ j ].index = module_add_fixed_var( module,
                        args[ j ].arg_name, counter );
            }
            result[ i ].args = args;
            result[ i ].size = formal[ i ].size;
        }

        return result;
    }
    return 0;
}


void argument_list_free( struct arg_list * args, int args_count )
{
    int i;
    for ( i = 0; i < args_count; ++i )
    {
        int j;
        for ( j = 0; j < args[ i ].size; ++j )
        {
            if ( args[ i ].args[ j ].type_name  )
                object_free( args[ i ].args[ j ].type_name );
            object_free( args[ i ].args[ j ].arg_name );
        }
        BJAM_FREE( args[ i ].args );
    }
    BJAM_FREE( args );
}


FUNCTION * function_unbind_variables( FUNCTION * f )
{
    if ( f->type == FUNCTION_JAM )
    {
        JAM_FUNCTION * const func = (JAM_FUNCTION *)f;
        return func->generic ? func->generic : f;
    }
#ifdef HAVE_PYTHON
    if ( f->type == FUNCTION_PYTHON )
        return f;
#endif
    assert( f->type == FUNCTION_BUILTIN );
    return f;
}

FUNCTION * function_bind_variables( FUNCTION * f, module_t * module,
    int * counter )
{
    if ( f->type == FUNCTION_BUILTIN )
        return f;
#ifdef HAVE_PYTHON
    if ( f->type == FUNCTION_PYTHON )
        return f;
#endif
    {
        JAM_FUNCTION * func = (JAM_FUNCTION *)f;
        JAM_FUNCTION * new_func = BJAM_MALLOC( sizeof( JAM_FUNCTION ) );
        instruction * code;
        int i;
        assert( f->type == FUNCTION_JAM );
        memcpy( new_func, func, sizeof( JAM_FUNCTION ) );
        new_func->base.reference_count = 1;
        new_func->base.formal_arguments = argument_list_bind_variables(
            f->formal_arguments, f->num_formal_arguments, module, counter );
        new_func->code = BJAM_MALLOC( func->code_size * sizeof( instruction ) );
        memcpy( new_func->code, func->code, func->code_size * sizeof(
            instruction ) );
        new_func->generic = (FUNCTION *)func;
        func = new_func;
        for ( i = 0; ; ++i )
        {
            OBJECT * key;
            int op_code;
            code = func->code + i;
            switch ( code->op_code )
            {
            case INSTR_PUSH_VAR: op_code = INSTR_PUSH_VAR_FIXED; break;
            case INSTR_PUSH_LOCAL: op_code = INSTR_PUSH_LOCAL_FIXED; break;
            case INSTR_POP_LOCAL: op_code = INSTR_POP_LOCAL_FIXED; break;
            case INSTR_SET: op_code = INSTR_SET_FIXED; break;
            case INSTR_APPEND: op_code = INSTR_APPEND_FIXED; break;
            case INSTR_DEFAULT: op_code = INSTR_DEFAULT_FIXED; break;
            case INSTR_RETURN: return (FUNCTION *)new_func;
            case INSTR_CALL_MEMBER_RULE:
            case INSTR_CALL_RULE: ++i; continue;
            case INSTR_PUSH_MODULE:
                {
                    int depth = 1;
                    ++i;
                    while ( depth > 0 )
                    {
                        code = func->code + i;
                        switch ( code->op_code )
                        {
                        case INSTR_PUSH_MODULE:
                        case INSTR_CLASS:
                            ++depth;
                            break;
                        case INSTR_POP_MODULE:
                            --depth;
                            break;
                        case INSTR_CALL_RULE:
                            ++i;
                            break;
                        }
                        ++i;
                    }
                    --i;
                }
            default: continue;
            }
            key = func->constants[ code->arg ];
            if ( !( object_equal( key, constant_TMPDIR ) ||
                    object_equal( key, constant_TMPNAME ) ||
                    object_equal( key, constant_TMPFILE ) ||
                    object_equal( key, constant_STDOUT ) ||
                    object_equal( key, constant_STDERR ) ) )
            {
                code->op_code = op_code;
                code->arg = module_add_fixed_var( module, key, counter );
            }
        }
    }
}

void function_refer( FUNCTION * func )
{
    ++func->reference_count;
}

void function_free( FUNCTION * function_ )
{
    int i;

    if ( --function_->reference_count != 0 )
        return;

    if ( function_->formal_arguments )
        argument_list_free( function_->formal_arguments,
            function_->num_formal_arguments );

    if ( function_->type == FUNCTION_JAM )
    {
        JAM_FUNCTION * func = (JAM_FUNCTION *)function_;

        BJAM_FREE( func->code );

        if ( func->generic )
            function_free( func->generic );
        else
        {
            if ( function_->rulename ) object_free( function_->rulename );

            for ( i = 0; i < func->num_constants; ++i )
                object_free( func->constants[ i ] );
            BJAM_FREE( func->constants );

            for ( i = 0; i < func->num_subfunctions; ++i )
            {
                object_free( func->functions[ i ].name );
                function_free( func->functions[ i ].code );
            }
            BJAM_FREE( func->functions );

            for ( i = 0; i < func->num_subactions; ++i )
            {
                object_free( func->actions[ i ].name );
                function_free( func->actions[ i ].command );
            }
            BJAM_FREE( func->actions );

            object_free( func->file );
        }
    }
#ifdef HAVE_PYTHON
    else if ( function_->type == FUNCTION_PYTHON )
    {
        PYTHON_FUNCTION * func = (PYTHON_FUNCTION *)function_;
        Py_DECREF( func->python_function );
        if ( function_->rulename ) object_free( function_->rulename );
    }
#endif
    else
    {
        assert( function_->type == FUNCTION_BUILTIN );
        if ( function_->rulename ) object_free( function_->rulename );
    }

    BJAM_FREE( function_ );
}


/* Alignment check for stack */

struct align_var_edits
{
    char ch;
    VAR_EDITS e;
};

struct align_expansion_item
{
    char ch;
    expansion_item e;
};

static char check_align_var_edits[ sizeof(struct align_var_edits) <= sizeof(VAR_EDITS) + sizeof(void *) ? 1 : -1 ];
static char check_align_expansion_item[ sizeof(struct align_expansion_item) <= sizeof(expansion_item) + sizeof(void *) ? 1 : -1 ];

static char check_ptr_size1[ sizeof(LIST *) <= sizeof(void *) ? 1 : -1 ];
static char check_ptr_size2[ sizeof(char *) <= sizeof(void *) ? 1 : -1 ];

void function_run_actions( FUNCTION * function, FRAME * frame, STACK * s,
    string * out )
{
    *(string * *)stack_allocate( s, sizeof( string * ) ) = out;
    list_free( function_run( function, frame, s ) );
    stack_deallocate( s, sizeof( string * ) );
}

/*
 * WARNING: The instruction set is tuned for Jam and is not really generic. Be
 * especially careful about stack push/pop.
 */

LIST * function_run( FUNCTION * function_, FRAME * frame, STACK * s )
{
    JAM_FUNCTION * function;
    instruction * code;
    LIST * l;
    LIST * r;
    LIST * result = L0;
    void * saved_stack = s->data;

    if ( function_->type == FUNCTION_BUILTIN )
    {
        BUILTIN_FUNCTION const * const f = (BUILTIN_FUNCTION *)function_;
        if ( function_->formal_arguments )
            argument_list_check( function_->formal_arguments,
                function_->num_formal_arguments, function_, frame );
        return f->func( frame, f->flags );
    }

#ifdef HAVE_PYTHON
    else if ( function_->type == FUNCTION_PYTHON )
    {
        PYTHON_FUNCTION * f = (PYTHON_FUNCTION *)function_;
        return call_python_function( f, frame );
    }
#endif

    assert( function_->type == FUNCTION_JAM );

    if ( function_->formal_arguments )
        argument_list_push( function_->formal_arguments,
            function_->num_formal_arguments, function_, frame, s );

    function = (JAM_FUNCTION *)function_;
    code = function->code;
    for ( ; ; )
    {
        switch ( code->op_code )
        {

        /*
         * Basic stack manipulation
         */

        case INSTR_PUSH_EMPTY:
            stack_push( s, L0 );
            break;

        case INSTR_PUSH_CONSTANT:
        {
            OBJECT * value = function_get_constant( function, code->arg );
            stack_push( s, list_new( object_copy( value ) ) );
            break;
        }

        case INSTR_PUSH_ARG:
            stack_push( s, frame_get_local( frame, code->arg ) );
            break;

        case INSTR_PUSH_VAR:
            stack_push( s, function_get_variable( function, frame, code->arg ) );
            break;

        case INSTR_PUSH_VAR_FIXED:
            stack_push( s, list_copy( frame->module->fixed_variables[ code->arg
                ] ) );
            break;

        case INSTR_PUSH_GROUP:
        {
            LIST * value = L0;
            LISTITER iter;
            LISTITER end;
            l = stack_pop( s );
            for ( iter = list_begin( l ), end = list_end( l ); iter != end;
                iter = list_next( iter ) )
                value = list_append( value, function_get_named_variable(
                    function, frame, list_item( iter ) ) );
            list_free( l );
            stack_push( s, value );
            break;
        }

        case INSTR_PUSH_APPEND:
            r = stack_pop( s );
            l = stack_pop( s );
            stack_push( s, list_append( l, r ) );
            break;

        case INSTR_SWAP:
            l = stack_top( s );
            stack_set( s, 0, stack_at( s, code->arg ) );
            stack_set( s, code->arg, l );
            break;

        case INSTR_POP:
            list_free( stack_pop( s ) );
            break;

        /*
         * Branch instructions
         */

        case INSTR_JUMP:
            code += code->arg;
            break;

        case INSTR_JUMP_EMPTY:
            l = stack_pop( s );
            if ( !list_cmp( l, L0 ) ) code += code->arg;
            list_free( l );
            break;

        case INSTR_JUMP_NOT_EMPTY:
            l = stack_pop( s );
            if ( list_cmp( l, L0 ) ) code += code->arg;
            list_free( l );
            break;

        case INSTR_JUMP_LT:
            r = stack_pop( s );
            l = stack_pop( s );
            if ( list_cmp( l, r ) < 0 ) code += code->arg;
            list_free( l );
            list_free( r );
            break;

        case INSTR_JUMP_LE:
            r = stack_pop( s );
            l = stack_pop( s );
            if ( list_cmp( l, r ) <= 0 ) code += code->arg;
            list_free( l );
            list_free( r );
            break;

        case INSTR_JUMP_GT:
            r = stack_pop( s );
            l = stack_pop( s );
            if ( list_cmp( l, r ) > 0 ) code += code->arg;
            list_free( l );
            list_free( r );
            break;

        case INSTR_JUMP_GE:
            r = stack_pop( s );
            l = stack_pop( s );
            if ( list_cmp( l, r ) >= 0 ) code += code->arg;
            list_free( l );
            list_free( r );
            break;

        case INSTR_JUMP_EQ:
            r = stack_pop( s );
            l = stack_pop( s );
            if ( list_cmp( l, r ) == 0 ) code += code->arg;
            list_free( l );
            list_free( r );
            break;

        case INSTR_JUMP_NE:
            r = stack_pop(s);
            l = stack_pop(s);
            if ( list_cmp(l, r) != 0 ) code += code->arg;
            list_free(l);
            list_free(r);
            break;

        case INSTR_JUMP_IN:
            r = stack_pop(s);
            l = stack_pop(s);
            if ( list_is_sublist( l, r ) ) code += code->arg;
            list_free(l);
            list_free(r);
            break;

        case INSTR_JUMP_NOT_IN:
            r = stack_pop( s );
            l = stack_pop( s );
            if ( !list_is_sublist( l, r ) ) code += code->arg;
            list_free( l );
            list_free( r );
            break;

        /*
         * For
         */

        case INSTR_FOR_INIT:
            l = stack_top( s );
            *(LISTITER *)stack_allocate( s, sizeof( LISTITER ) ) =
                list_begin( l );
            break;

        case INSTR_FOR_LOOP:
        {
            LISTITER iter = *(LISTITER *)stack_get( s );
            stack_deallocate( s, sizeof( LISTITER ) );
            l = stack_top( s );
            if ( iter == list_end( l ) )
            {
                list_free( stack_pop( s ) );
                code += code->arg;
            }
            else
            {
                r = list_new( object_copy( list_item( iter ) ) );
                iter = list_next( iter );
                *(LISTITER *)stack_allocate( s, sizeof( LISTITER ) ) = iter;
                stack_push( s, r );
            }
            break;
        }

        /*
         * Switch
         */

        case INSTR_JUMP_NOT_GLOB:
        {
            char const * pattern;
            char const * match;
            l = stack_pop( s );
            r = stack_top( s );
            pattern = list_empty( l ) ? "" : object_str( list_front( l ) );
            match = list_empty( r ) ? "" : object_str( list_front( r ) );
            if ( glob( pattern, match ) )
                code += code->arg;
            else
                list_free( stack_pop( s ) );
            list_free( l );
            break;
        }

        /*
         * Return
         */

        case INSTR_SET_RESULT:
            list_free( result );
            if ( !code->arg )
                result = stack_pop( s );
            else
                result = list_copy( stack_top( s ) );
            break;

        case INSTR_PUSH_RESULT:
            stack_push( s, result );
            result = L0;
            break;

        case INSTR_RETURN:
        {
            if ( function_->formal_arguments )
                argument_list_pop( function_->formal_arguments,
                    function_->num_formal_arguments, frame, s );
#ifndef NDEBUG
            if ( !( saved_stack == s->data ) )
            {
                frame->file = function->file;
                frame->line = function->line;
                backtrace_line( frame );
                printf( "error: stack check failed.\n" );
                backtrace( frame );
                assert( saved_stack == s->data );
            }
#endif
            assert( saved_stack == s->data );
            return result;
        }

        /*
         * Local variables
         */

        case INSTR_PUSH_LOCAL:
        {
            LIST * value = stack_pop( s );
            stack_push( s, function_swap_variable( function, frame, code->arg,
                value ) );
            break;
        }

        case INSTR_POP_LOCAL:
            function_set_variable( function, frame, code->arg, stack_pop( s ) );
            break;

        case INSTR_PUSH_LOCAL_FIXED:
        {
            LIST * value = stack_pop( s );
            LIST * * ptr = &frame->module->fixed_variables[ code->arg ];
            assert( code->arg < frame->module->num_fixed_variables );
            stack_push( s, *ptr );
            *ptr = value;
            break;
        }

        case INSTR_POP_LOCAL_FIXED:
        {
            LIST * value = stack_pop( s );
            LIST * * ptr = &frame->module->fixed_variables[ code->arg ];
            assert( code->arg < frame->module->num_fixed_variables );
            list_free( *ptr );
            *ptr = value;
            break;
        }

        case INSTR_PUSH_LOCAL_GROUP:
        {
            LIST * const value = stack_pop( s );
            LISTITER iter;
            LISTITER end;
            l = stack_pop( s );
            for ( iter = list_begin( l ), end = list_end( l ); iter != end;
                iter = list_next( iter ) )
                stack_push( s, function_swap_named_variable( function, frame,
                    list_item( iter ), list_copy( value ) ) );
            list_free( value );
            stack_push( s, l );
            break;
        }

        case INSTR_POP_LOCAL_GROUP:
        {
            LISTITER iter;
            LISTITER end;
            r = stack_pop( s );
            l = list_reverse( r );
            list_free( r );
            for ( iter = list_begin( l ), end = list_end( l ); iter != end;
                iter = list_next( iter ) )
                function_set_named_variable( function, frame, list_item( iter ),
                    stack_pop( s ) );
            list_free( l );
            break;
        }

        /*
         * on $(TARGET) variables
         */

        case INSTR_PUSH_ON:
        {
            LIST * targets = stack_top( s );
            if ( !list_empty( targets ) )
            {
                /* FIXME: push the state onto the stack instead of using
                 * pushsettings.
                 */
                TARGET * t = bindtarget( list_front( targets ) );
                pushsettings( frame->module, t->settings );
            }
            else
            {
                /* [ on $(TARGET) ... ] is ignored if $(TARGET) is empty. */
                list_free( stack_pop( s ) );
                stack_push( s, L0 );
                code += code->arg;
            }
            break;
        }

        case INSTR_POP_ON:
        {
            LIST * result = stack_pop( s );
            LIST * targets = stack_pop( s );
            if ( !list_empty( targets ) )
            {
                TARGET * t = bindtarget( list_front( targets ) );
                popsettings( frame->module, t->settings );
            }
            list_free( targets );
            stack_push( s, result );
            break;
        }

        case INSTR_SET_ON:
        {
            LIST * targets = stack_pop( s );
            LIST * value = stack_pop( s );
            LIST * vars = stack_pop( s );
            LISTITER iter = list_begin( targets );
            LISTITER const end = list_end( targets );
            for ( ; iter != end; iter = list_next( iter ) )
            {
                TARGET * t = bindtarget( list_item( iter ) );
                LISTITER vars_iter = list_begin( vars );
                LISTITER const vars_end = list_end( vars );
                for ( ; vars_iter != vars_end; vars_iter = list_next( vars_iter
                    ) )
                    t->settings = addsettings( t->settings, VAR_SET, list_item(
                        vars_iter ), list_copy( value ) );
            }
            list_free( vars );
            list_free( targets );
            stack_push( s, value );
            break;
        }

        case INSTR_APPEND_ON:
        {
            LIST * targets = stack_pop( s );
            LIST * value = stack_pop( s );
            LIST * vars = stack_pop( s );
            LISTITER iter = list_begin( targets );
            LISTITER const end = list_end( targets );
            for ( ; iter != end; iter = list_next( iter ) )
            {
                TARGET * const t = bindtarget( list_item( iter ) );
                LISTITER vars_iter = list_begin( vars );
                LISTITER const vars_end = list_end( vars );
                for ( ; vars_iter != vars_end; vars_iter = list_next( vars_iter
                    ) )
                    t->settings = addsettings( t->settings, VAR_APPEND,
                        list_item( vars_iter ), list_copy( value ) );
            }
            list_free( vars );
            list_free( targets );
            stack_push( s, value );
            break;
        }

        case INSTR_DEFAULT_ON:
        {
            LIST * targets = stack_pop( s );
            LIST * value = stack_pop( s );
            LIST * vars = stack_pop( s );
            LISTITER iter = list_begin( targets );
            LISTITER const end = list_end( targets );
            for ( ; iter != end; iter = list_next( iter ) )
            {
                TARGET * t = bindtarget( list_item( iter ) );
                LISTITER vars_iter = list_begin( vars );
                LISTITER const vars_end = list_end( vars );
                for ( ; vars_iter != vars_end; vars_iter = list_next( vars_iter
                    ) )
                    t->settings = addsettings( t->settings, VAR_DEFAULT,
                        list_item( vars_iter ), list_copy( value ) );
            }
            list_free( vars );
            list_free( targets );
            stack_push( s, value );
            break;
        }

        /* [ on $(target) return $(variable) ] */
        case INSTR_GET_ON:
        {
            LIST * targets = stack_pop( s );
            LIST * result = L0;
            if ( !list_empty( targets ) )
            {
                OBJECT * varname = function->constants[ code->arg ];
                TARGET * t = bindtarget( list_front( targets ) );
                SETTINGS * s = t->settings;
                int found = 0;
                for ( ; s != 0; s = s->next )
                {
                    if ( object_equal( s->symbol, varname ) )
                    {
                        result = s->value;
                        found = 1;
                        break;
                    }
                }
                if ( !found )
                {
                    result = var_get( frame->module, varname ) ;
                }
            }
            stack_push( s, list_copy( result ) );
            break;
        }

        /*
         * Variable setting
         */

        case INSTR_SET:
            function_set_variable( function, frame, code->arg,
                stack_pop( s ) );
            break;

        case INSTR_APPEND:
            function_append_variable( function, frame, code->arg,
                stack_pop( s ) );
            break;

        case INSTR_DEFAULT:
            function_default_variable( function, frame, code->arg,
                stack_pop( s ) );
            break;

        case INSTR_SET_FIXED:
        {
            LIST * * ptr = &frame->module->fixed_variables[ code->arg ];
            assert( code->arg < frame->module->num_fixed_variables );
            list_free( *ptr );
            *ptr = stack_pop( s );
            break;
        }

        case INSTR_APPEND_FIXED:
        {
            LIST * * ptr = &frame->module->fixed_variables[ code->arg ];
            assert( code->arg < frame->module->num_fixed_variables );
            *ptr = list_append( *ptr, stack_pop( s ) );
            break;
        }

        case INSTR_DEFAULT_FIXED:
        {
            LIST * * ptr = &frame->module->fixed_variables[ code->arg ];
            LIST * value = stack_pop( s );
            assert( code->arg < frame->module->num_fixed_variables );
            if ( list_empty( *ptr ) )
                *ptr = value;
            else
                list_free( value );
            break;
        }

        case INSTR_SET_GROUP:
        {
            LIST * value = stack_pop( s );
            LIST * vars = stack_pop( s );
            LISTITER iter = list_begin( vars );
            LISTITER const end = list_end( vars );
            for ( ; iter != end; iter = list_next( iter ) )
                function_set_named_variable( function, frame, list_item( iter ),
                    list_copy( value ) );
            list_free( vars );
            list_free( value );
            break;
        }

        case INSTR_APPEND_GROUP:
        {
            LIST * value = stack_pop( s );
            LIST * vars = stack_pop( s );
            LISTITER iter = list_begin( vars );
            LISTITER const end = list_end( vars );
            for ( ; iter != end; iter = list_next( iter ) )
                function_append_named_variable( function, frame, list_item( iter
                    ), list_copy( value ) );
            list_free( vars );
            list_free( value );
            break;
        }

        case INSTR_DEFAULT_GROUP:
        {
            LIST * value = stack_pop( s );
            LIST * vars = stack_pop( s );
            LISTITER iter = list_begin( vars );
            LISTITER const end = list_end( vars );
            for ( ; iter != end; iter = list_next( iter ) )
                function_default_named_variable( function, frame, list_item(
                    iter ), list_copy( value ) );
            list_free( vars );
            list_free( value );
            break;
        }

        /*
         * Rules
         */

        case INSTR_CALL_RULE:
        {
            char const * unexpanded = object_str( function_get_constant(
                function, code[ 1 ].op_code ) );
            LIST * result = function_call_rule( function, frame, s, code->arg,
                unexpanded, function->file, code[ 1 ].arg );
            stack_push( s, result );
            ++code;
            break;
        }

        case INSTR_CALL_MEMBER_RULE:
        {
            OBJECT * rule_name = function_get_constant( function, code[1].op_code );
            LIST * result = function_call_member_rule( function, frame, s, code->arg, rule_name, function->file, code[1].arg );
            stack_push( s, result );
            ++code;
            break;
        }

        case INSTR_RULE:
            function_set_rule( function, frame, s, code->arg );
            break;

        case INSTR_ACTIONS:
            function_set_actions( function, frame, s, code->arg );
            break;

        /*
         * Variable expansion
         */

        case INSTR_APPLY_MODIFIERS:
        {
            int n;
            int i;
            l = stack_pop( s );
            n = expand_modifiers( s, code->arg );
            stack_push( s, l );
            l = apply_modifiers( s, n );
            list_free( stack_pop( s ) );
            stack_deallocate( s, n * sizeof( VAR_EDITS ) );
            for ( i = 0; i < code->arg; ++i )
                list_free( stack_pop( s ) );  /* pop modifiers */
            stack_push( s, l );
            break;
        }

        case INSTR_APPLY_INDEX:
            l = apply_subscript( s );
            list_free( stack_pop( s ) );
            list_free( stack_pop( s ) );
            stack_push( s, l );
            break;

        case INSTR_APPLY_INDEX_MODIFIERS:
        {
            int i;
            int n;
            l = stack_pop( s );
            r = stack_pop( s );
            n = expand_modifiers( s, code->arg );
            stack_push( s, r );
            stack_push( s, l );
            l = apply_subscript_and_modifiers( s, n );
            list_free( stack_pop( s ) );
            list_free( stack_pop( s ) );
            stack_deallocate( s, n * sizeof( VAR_EDITS ) );
            for ( i = 0; i < code->arg; ++i )
                list_free( stack_pop( s ) );  /* pop modifiers */
            stack_push( s, l );
            break;
        }

        case INSTR_APPLY_MODIFIERS_GROUP:
        {
            int i;
            LIST * const vars = stack_pop( s );
            int const n = expand_modifiers( s, code->arg );
            LIST * result = L0;
            LISTITER iter = list_begin( vars );
            LISTITER const end = list_end( vars );
            for ( ; iter != end; iter = list_next( iter ) )
            {
                stack_push( s, function_get_named_variable( function, frame,
                    list_item( iter ) ) );
                result = list_append( result, apply_modifiers( s, n ) );
                list_free( stack_pop( s ) );
            }
            list_free( vars );
            stack_deallocate( s, n * sizeof( VAR_EDITS ) );
            for ( i = 0; i < code->arg; ++i )
                list_free( stack_pop( s ) );  /* pop modifiers */
            stack_push( s, result );
            break;
        }

        case INSTR_APPLY_INDEX_GROUP:
        {
            LIST * vars = stack_pop( s );
            LIST * result = L0;
            LISTITER iter = list_begin( vars );
            LISTITER const end = list_end( vars );
            for ( ; iter != end; iter = list_next( iter ) )
            {
                stack_push( s, function_get_named_variable( function, frame,
                    list_item( iter ) ) );
                result = list_append( result, apply_subscript( s ) );
                list_free( stack_pop( s ) );
            }
            list_free( vars );
            list_free( stack_pop( s ) );
            stack_push( s, result );
            break;
        }

        case INSTR_APPLY_INDEX_MODIFIERS_GROUP:
        {
            int i;
            LIST * const vars = stack_pop( s );
            LIST * const r = stack_pop( s );
            int const n = expand_modifiers( s, code->arg );
            LIST * result = L0;
            LISTITER iter = list_begin( vars );
            LISTITER const end = list_end( vars );
            stack_push( s, r );
            for ( ; iter != end; iter = list_next( iter ) )
            {
                stack_push( s, function_get_named_variable( function, frame,
                    list_item( iter ) ) );
                result = list_append( result, apply_subscript_and_modifiers( s,
                    n ) );
                list_free( stack_pop( s ) );
            }
            list_free( stack_pop( s ) );
            list_free( vars );
            stack_deallocate( s, n * sizeof( VAR_EDITS ) );
            for ( i = 0; i < code->arg; ++i )
                list_free( stack_pop( s ) );  /* pop modifiers */
            stack_push( s, result );
            break;
        }

        case INSTR_COMBINE_STRINGS:
        {
            size_t const buffer_size = code->arg * sizeof( expansion_item );
            LIST * * const stack_pos = stack_get( s );
            expansion_item * items = stack_allocate( s, buffer_size );
            LIST * result;
            int i;
            for ( i = 0; i < code->arg; ++i )
                items[ i ].values = stack_pos[ i ];
            result = expand( items, code->arg );
            stack_deallocate( s, buffer_size );
            for ( i = 0; i < code->arg; ++i )
                list_free( stack_pop( s ) );
            stack_push( s, result );
            break;
        }

        case INSTR_GET_GRIST:
        {
            LIST * vals = stack_pop( s );
            LIST * result = L0;
            LISTITER iter, end;

            for ( iter = list_begin( vals ), end = list_end( vals ); iter != end; ++iter )
            {
                OBJECT * new_object;
                const char * value = object_str( list_item( iter ) );
                const char * p;
                if ( value[ 0 ] == '<' && ( p = strchr( value, '>' ) ) )
                {
                    if( p[ 1 ] )
                        new_object = object_new_range( value, p - value + 1 );
                    else
                        new_object = object_copy( list_item( iter ) );
                }
                else
                {
                    new_object = object_copy( constant_empty );
                }
                result = list_push_back( result, new_object );
            }

            list_free( vals );
            stack_push( s, result );
            break;
        }

        case INSTR_INCLUDE:
        {
            LIST * nt = stack_pop( s );
            if ( !list_empty( nt ) )
            {
                TARGET * const t = bindtarget( list_front( nt ) );
                list_free( nt );

                /* DWA 2001/10/22 - Perforce Jam cleared the arguments here,
                 * which prevented an included file from being treated as part
                 * of the body of a rule. I did not see any reason to do that,
                 * so I lifted the restriction.
                 */

                /* Bind the include file under the influence of "on-target"
                 * variables. Though they are targets, include files are not
                 * built with make().
                 */

                pushsettings( root_module(), t->settings );
                /* We do not expect that a file to be included is generated by
                 * some action. Therefore, pass 0 as third argument. If the name
                 * resolves to a directory, let it error out.
                 */
                object_free( t->boundname );
                t->boundname = search( t->name, &t->time, 0, 0 );
                popsettings( root_module(), t->settings );

                parse_file( t->boundname, frame );
            }
            break;
        }

        /*
         * Classes and modules
         */

        case INSTR_PUSH_MODULE:
        {
            LIST * const module_name = stack_pop( s );
            module_t * const outer_module = frame->module;
            frame->module = !list_empty( module_name )
                ? bindmodule( list_front( module_name ) )
                : root_module();
            list_free( module_name );
            *(module_t * *)stack_allocate( s, sizeof( module_t * ) ) =
                outer_module;
            break;
        }

        case INSTR_POP_MODULE:
        {
            module_t * const outer_module = *(module_t * *)stack_get( s );
            stack_deallocate( s, sizeof( module_t * ) );
            frame->module = outer_module;
            break;
        }

        case INSTR_CLASS:
        {
            LIST * bases = stack_pop( s );
            LIST * name = stack_pop( s );
            OBJECT * class_module = make_class_module( name, bases, frame );

            module_t * const outer_module = frame->module;
            frame->module = bindmodule( class_module );
            object_free( class_module );

            *(module_t * *)stack_allocate( s, sizeof( module_t * ) ) =
                outer_module;
            break;
        }

        case INSTR_BIND_MODULE_VARIABLES:
            module_bind_variables( frame->module );
            break;

        case INSTR_APPEND_STRINGS:
        {
            string buf[ 1 ];
            string_new( buf );
            combine_strings( s, code->arg, buf );
            stack_push( s, list_new( object_new( buf->value ) ) );
            string_free( buf );
            break;
        }

        case INSTR_WRITE_FILE:
        {
            string buf[ 1 ];
            char const * out;
            OBJECT * tmp_filename = 0;
            int out_debug = DEBUG_EXEC ? 1 : 0;
            FILE * out_file = 0;
            string_new( buf );
            combine_strings( s, code->arg, buf );
            out = object_str( list_front( stack_top( s ) ) );

            /* For stdout/stderr we will create a temp file and generate a
             * command that outputs the content as needed.
             */
            if ( ( strcmp( "STDOUT", out ) == 0 ) ||
                ( strcmp( "STDERR", out ) == 0 ) )
            {
                int err_redir = strcmp( "STDERR", out ) == 0;
                string result[ 1 ];
                tmp_filename = path_tmpfile();
                string_new( result );
                #ifdef OS_NT
                string_append( result, "type \"" );
                #else
                string_append( result, "cat \"" );
                #endif
                string_append( result, object_str( tmp_filename ) );
                string_push_back( result, '\"' );
                if ( err_redir )
                    string_append( result, " 1>&2" );

                /* Replace STDXXX with the temporary file. */
                list_free( stack_pop( s ) );
                stack_push( s, list_new( object_new( result->value ) ) );
                out = object_str( tmp_filename );

                string_free( result );

                /* Make sure temp files created by this get nuked eventually. */
                file_remove_atexit( tmp_filename );
            }

            if ( !globs.noexec )
            {
                string out_name[ 1 ];
                /* Handle "path to file" filenames. */
                if ( ( out[ 0 ] == '"' ) && ( out[ strlen( out ) - 1 ] == '"' )
                    )
                {
                    string_copy( out_name, out + 1 );
                    string_truncate( out_name, out_name->size - 1 );
                }
                else
                    string_copy( out_name, out );
                out_file = fopen( out_name->value, "w" );

                if ( !out_file )
                {
                    printf( "failed to write output file '%s'!\n",
                        out_name->value );
                    exit( EXITBAD );
                }
                string_free( out_name );
            }

            if ( out_debug ) printf( "\nfile %s\n", out );
            if ( out_file ) fputs( buf->value, out_file );
            if ( out_debug ) fputs( buf->value, stdout );
            if ( out_file )
            {
                fflush( out_file );
                fclose( out_file );
            }
            string_free( buf );
            if ( tmp_filename )
                object_free( tmp_filename );

            if ( out_debug ) fputc( '\n', stdout );
            break;
        }

        case INSTR_OUTPUT_STRINGS:
        {
            string * const buf = *(string * *)( (char *)stack_get( s ) + (
                code->arg * sizeof( LIST * ) ) );
            combine_strings( s, code->arg, buf );
            break;
        }

        }
        ++code;
    }
}


#ifdef HAVE_PYTHON

static struct arg_list * arg_list_compile_python( PyObject * bjam_signature,
    int * num_arguments )
{
    if ( bjam_signature )
    {
        struct argument_list_compiler c[ 1 ];
        struct arg_list * result;
        Py_ssize_t s;
        Py_ssize_t i;
        argument_list_compiler_init( c );

        s = PySequence_Size( bjam_signature );
        for ( i = 0; i < s; ++i )
        {
            struct argument_compiler arg_comp[ 1 ];
            struct arg_list arg;
            PyObject * v = PySequence_GetItem( bjam_signature, i );
            Py_ssize_t j;
            Py_ssize_t inner;
            argument_compiler_init( arg_comp );

            inner = PySequence_Size( v );
            for ( j = 0; j < inner; ++j )
                argument_compiler_add( arg_comp, object_new( PyString_AsString(
                    PySequence_GetItem( v, j ) ) ), constant_builtin, -1 );

            arg = arg_compile_impl( arg_comp, constant_builtin, -1 );
            dynamic_array_push( c->args, arg );
            argument_compiler_free( arg_comp );
            Py_DECREF( v );
        }

        *num_arguments = c->args->size;
        result = BJAM_MALLOC( c->args->size * sizeof( struct arg_list ) );
        memcpy( result, c->args->data, c->args->size * sizeof( struct arg_list )
            );
        argument_list_compiler_free( c );
        return result;
    }
    *num_arguments = 0;
    return 0;
}

FUNCTION * function_python( PyObject * function, PyObject * bjam_signature )
{
    PYTHON_FUNCTION * result = BJAM_MALLOC( sizeof( PYTHON_FUNCTION ) );

    result->base.type = FUNCTION_PYTHON;
    result->base.reference_count = 1;
    result->base.rulename = 0;
    result->base.formal_arguments = arg_list_compile_python( bjam_signature,
        &result->base.num_formal_arguments );
    Py_INCREF( function );
    result->python_function = function;

    return (FUNCTION *)result;
}


static void argument_list_to_python( struct arg_list * formal, int formal_count,
    FUNCTION * function, FRAME * frame, PyObject * kw )
{
    LOL * all_actual = frame->args;
    int i;

    for ( i = 0; i < formal_count; ++i )
    {
        LIST * actual = lol_get( all_actual, i );
        LISTITER actual_iter = list_begin( actual );
        LISTITER const actual_end = list_end( actual );
        int j;
        for ( j = 0; j < formal[ i ].size; ++j )
        {
            struct argument * formal_arg = &formal[ i ].args[ j ];
            PyObject * value;
            LIST * l;

            switch ( formal_arg->flags )
            {
            case ARG_ONE:
                if ( actual_iter == actual_end )
                    argument_error( "missing argument", function, frame,
                        formal_arg->arg_name );
                type_check_range( formal_arg->type_name, actual_iter, list_next(
                    actual_iter ), frame, function, formal_arg->arg_name );
                value = PyString_FromString( object_str( list_item( actual_iter
                    ) ) );
                actual_iter = list_next( actual_iter );
                break;
            case ARG_OPTIONAL:
                if ( actual_iter == actual_end )
                    value = 0;
                else
                {
                    type_check_range( formal_arg->type_name, actual_iter,
                        list_next( actual_iter ), frame, function,
                        formal_arg->arg_name );
                    value = PyString_FromString( object_str( list_item(
                        actual_iter ) ) );
                    actual_iter = list_next( actual_iter );
                }
                break;
            case ARG_PLUS:
                if ( actual_iter == actual_end )
                    argument_error( "missing argument", function, frame,
                        formal_arg->arg_name );
                /* fallthrough */
            case ARG_STAR:
                type_check_range( formal_arg->type_name, actual_iter,
                    actual_end, frame, function, formal_arg->arg_name );
                l = list_copy_range( actual, actual_iter, actual_end );
                value = list_to_python( l );
                list_free( l );
                actual_iter = actual_end;
                break;
            case ARG_VARIADIC:
                return;
            }

            if ( value )
            {
                PyObject * key = PyString_FromString( object_str(
                    formal_arg->arg_name ) );
                PyDict_SetItem( kw, key, value );
                Py_DECREF( key );
                Py_DECREF( value );
            }
        }

        if ( actual_iter != actual_end )
            argument_error( "extra argument", function, frame, list_item(
                actual_iter ) );
    }

    for ( ; i < all_actual->count; ++i )
    {
        LIST * const actual = lol_get( all_actual, i );
        if ( !list_empty( actual ) )
            argument_error( "extra argument", function, frame, list_front(
                actual ) );
    }
}


/* Given a Python object, return a string to use in Jam code instead of the said
 * object.
 *
 * If the object is a string, use the string value.
 * If the object implemenets __jam_repr__ method, use that.
 * Otherwise return 0.
 */

OBJECT * python_to_string( PyObject * value )
{
    if ( PyString_Check( value ) )
        return object_new( PyString_AS_STRING( value ) );

    /* See if this instance defines the special __jam_repr__ method. */
    if ( PyInstance_Check( value )
        && PyObject_HasAttrString( value, "__jam_repr__" ) )
    {
        PyObject * repr = PyObject_GetAttrString( value, "__jam_repr__" );
        if ( repr )
        {
            PyObject * arguments2 = PyTuple_New( 0 );
            PyObject * value2 = PyObject_Call( repr, arguments2, 0 );
            Py_DECREF( repr );
            Py_DECREF( arguments2 );
            if ( PyString_Check( value2 ) )
                return object_new( PyString_AS_STRING( value2 ) );
            Py_DECREF( value2 );
        }
    }
    return 0;
}


static module_t * python_module()
{
    static module_t * python = 0;
    if ( !python )
        python = bindmodule( constant_python );
    return python;
}


static LIST * call_python_function( PYTHON_FUNCTION * function, FRAME * frame )
{
    LIST * result = 0;
    PyObject * arguments = 0;
    PyObject * kw = NULL;
    int i;
    PyObject * py_result;
    FRAME * prev_frame_before_python_call;

    if ( function->base.formal_arguments )
    {
        arguments = PyTuple_New( 0 );
        kw = PyDict_New();
        argument_list_to_python( function->base.formal_arguments,
            function->base.num_formal_arguments, &function->base, frame, kw );
    }
    else
    {
        arguments = PyTuple_New( frame->args->count );
        for ( i = 0; i < frame->args->count; ++i )
            PyTuple_SetItem( arguments, i, list_to_python( lol_get( frame->args,
                i ) ) );
    }

    frame->module = python_module();

    prev_frame_before_python_call = frame_before_python_call;
    frame_before_python_call = frame;
    py_result = PyObject_Call( function->python_function, arguments, kw );
    frame_before_python_call = prev_frame_before_python_call;
    Py_DECREF( arguments );
    Py_XDECREF( kw );
    if ( py_result != NULL )
    {
        if ( PyList_Check( py_result ) )
        {
            int size = PyList_Size( py_result );
            int i;
            for ( i = 0; i < size; ++i )
            {
                OBJECT * s = python_to_string( PyList_GetItem( py_result, i ) );
                if ( !s )
                    fprintf( stderr,
                        "Non-string object returned by Python call.\n" );
                else
                    result = list_push_back( result, s );
            }
        }
        else if ( py_result == Py_None )
        {
            result = L0;
        }
        else
        {
            OBJECT * const s = python_to_string( py_result );
            if ( s )
                result = list_new( s );
            else
                /* We have tried all we could. Return empty list. There are
                 * cases, e.g. feature.feature function that should return a
                 * value for the benefit of Python code and which also can be
                 * called by Jam code, where no sensible value can be returned.
                 * We cannot even emit a warning, since there would be a pile of
                 * them.
                 */
                result = L0;
        }

        Py_DECREF( py_result );
    }
    else
    {
        PyErr_Print();
        fprintf( stderr, "Call failed\n" );
    }

    return result;
}

#endif


void function_done( void )
{
    BJAM_FREE( stack );
}
