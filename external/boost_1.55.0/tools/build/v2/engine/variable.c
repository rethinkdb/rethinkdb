/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/* This file is ALSO:
 * Copyright 2001-2004 David Abrahams.
 * Copyright 2005 Reece H. Dunn.
 * Copyright 2005 Rene Rivera.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * variable.c - handle Jam multi-element variables.
 *
 * External routines:
 *
 *  var_defines() - load a bunch of variable=value settings
 *  var_get()     - get value of a user defined symbol
 *  var_set()     - set a variable in jam's user defined symbol table.
 *  var_swap()    - swap a variable's value with the given one
 *  var_done()    - free variable tables
 *
 * Internal routines:
 *
 *  var_enter() - make new var symbol table entry, returning var ptr
 *  var_dump()  - dump a variable to stdout
 */

#include "jam.h"
#include "variable.h"

#include "filesys.h"
#include "hash.h"
#include "modules.h"
#include "parse.h"
#include "pathsys.h"
#include "strings.h"

#include <stdio.h>
#include <stdlib.h>


/*
 * VARIABLE - a user defined multi-value variable
 */

typedef struct _variable VARIABLE ;

struct _variable
{
    OBJECT * symbol;
    LIST   * value;
};

static LIST * * var_enter( struct module_t *, OBJECT * symbol );
static void var_dump( OBJECT * symbol, LIST * value, char * what );


/*
 * var_defines() - load a bunch of variable=value settings
 *
 * If preprocess is false, take the value verbatim.
 *
 * Otherwise, if the variable value is enclosed in quotes, strip the quotes.
 * Otherwise, if variable name ends in PATH, split value at :'s.
 * Otherwise, split the value at blanks.
 */

void var_defines( struct module_t * module, char * const * e, int preprocess )
{
    string buf[ 1 ];

    string_new( buf );

    for ( ; *e; ++e )
    {
        char * val;

        if ( ( val = strchr( *e, '=' ) )
#if defined( OS_MAC )
            /* On the mac (MPW), the var=val is actually var\0val */
            /* Think different. */
            || ( val = *e + strlen( *e ) )
#endif
        )
        {
            LIST * l = L0;
            size_t const len = strlen( val + 1 );
            int const quoted = ( val[ 1 ] == '"' ) && ( val[ len ] == '"' ) &&
                ( len > 1 );

            if ( quoted && preprocess )
            {
                string_append_range( buf, val + 2, val + len );
                l = list_push_back( l, object_new( buf->value ) );
                string_truncate( buf, 0 );
            }
            else
            {
                char * p;
                char * pp;
                char split =
#if defined( OPT_NO_EXTERNAL_VARIABLE_SPLIT )
                    '\0'
#elif defined( OS_MAC )
                    ','
#else
                    ' '
#endif
                    ;

                /* Split *PATH at :'s, not spaces. */
                if ( val - 4 >= *e )
                {
                    if ( !strncmp( val - 4, "PATH", 4 ) ||
                        !strncmp( val - 4, "Path", 4 ) ||
                        !strncmp( val - 4, "path", 4 ) )
                        split = SPLITPATH;
                }

                /* Do the split. */
                for
                (
                    pp = val + 1;
                    preprocess && ( ( p = strchr( pp, split ) ) != 0 );
                    pp = p + 1
                )
                {
                    string_append_range( buf, pp, p );
                    l = list_push_back( l, object_new( buf->value ) );
                    string_truncate( buf, 0 );
                }

                l = list_push_back( l, object_new( pp ) );
            }

            /* Get name. */
            string_append_range( buf, *e, val );
            {
                OBJECT * const varname = object_new( buf->value );
                var_set( module, varname, l, VAR_SET );
                object_free( varname );
            }
            string_truncate( buf, 0 );
        }
    }
    string_free( buf );
}


/* Last returned variable value saved so we may clear it in var_done(). */
static LIST * saved_var = L0;


/*
 * var_get() - get value of a user defined symbol
 *
 * Returns NULL if symbol unset.
 */

LIST * var_get( struct module_t * module, OBJECT * symbol )
{
    LIST * result = L0;
#ifdef OPT_AT_FILES
    /* Some "fixed" variables... */
    if ( object_equal( symbol, constant_TMPDIR ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( object_new( path_tmpdir()->value ) );
    }
    else if ( object_equal( symbol, constant_TMPNAME ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( path_tmpnam() );
    }
    else if ( object_equal( symbol, constant_TMPFILE ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( path_tmpfile() );
    }
    else if ( object_equal( symbol, constant_STDOUT ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( object_copy( constant_STDOUT ) );
    }
    else if ( object_equal( symbol, constant_STDERR ) )
    {
        list_free( saved_var );
        result = saved_var = list_new( object_copy( constant_STDERR ) );
    }
    else
#endif
    {
        VARIABLE * v;
        int n;

        if ( ( n = module_get_fixed_var( module, symbol ) ) != -1 )
        {
            if ( DEBUG_VARGET )
                var_dump( symbol, module->fixed_variables[ n ], "get" );
            result = module->fixed_variables[ n ];
        }
        else if ( module->variables && ( v = (VARIABLE *)hash_find(
            module->variables, symbol ) ) )
        {
            if ( DEBUG_VARGET )
                var_dump( v->symbol, v->value, "get" );
            result = v->value;
        }
    }
    return result;
}


LIST * var_get_and_clear_raw( module_t * module, OBJECT * symbol )
{
    LIST * result = L0;
    VARIABLE * v;

    if ( module->variables && ( v = (VARIABLE *)hash_find( module->variables,
        symbol ) ) )
    {
        result = v->value;
        v->value = L0;
    }

    return result;
}


/*
 * var_set() - set a variable in Jam's user defined symbol table
 *
 * 'flag' controls the relationship between new and old values of the variable:
 * SET replaces the old with the new; APPEND appends the new to the old; DEFAULT
 * only uses the new if the variable was previously unset.
 *
 * Copies symbol. Takes ownership of value.
 */

void var_set( struct module_t * module, OBJECT * symbol, LIST * value, int flag
    )
{
    LIST * * v = var_enter( module, symbol );

    if ( DEBUG_VARSET )
        var_dump( symbol, value, "set" );

    switch ( flag )
    {
    case VAR_SET:  /* Replace value */
        list_free( *v );
        *v = value;
        break;

    case VAR_APPEND:  /* Append value */
        *v = list_append( *v, value );
        break;

    case VAR_DEFAULT:  /* Set only if unset */
        if ( list_empty( *v ) )
            *v = value;
        else
            list_free( value );
        break;
    }
}


/*
 * var_swap() - swap a variable's value with the given one
 */

LIST * var_swap( struct module_t * module, OBJECT * symbol, LIST * value )
{
    LIST * * v = var_enter( module, symbol );
    LIST * oldvalue = *v;
    if ( DEBUG_VARSET )
        var_dump( symbol, value, "set" );
    *v = value;
    return oldvalue;
}


/*
 * var_enter() - make new var symbol table entry, returning var ptr
 */

static LIST * * var_enter( struct module_t * module, OBJECT * symbol )
{
    int found;
    VARIABLE * v;
    int n;

    if ( ( n = module_get_fixed_var( module, symbol ) ) != -1 )
        return &module->fixed_variables[ n ];

    if ( !module->variables )
        module->variables = hashinit( sizeof( VARIABLE ), "variables" );

    v = (VARIABLE *)hash_insert( module->variables, symbol, &found );
    if ( !found )
    {
        v->symbol = object_copy( symbol );
        v->value = L0;
    }

    return &v->value;
}


/*
 * var_dump() - dump a variable to stdout
 */

static void var_dump( OBJECT * symbol, LIST * value, char * what )
{
    printf( "%s %s = ", what, object_str( symbol ) );
    list_print( value );
    printf( "\n" );
}


/*
 * var_done() - free variable tables
 */

static void delete_var_( void * xvar, void * data )
{
    VARIABLE * const v = (VARIABLE *)xvar;
    object_free( v->symbol );
    list_free( v->value );
}

void var_done( struct module_t * module )
{
    list_free( saved_var );
    saved_var = L0;
    hashenumerate( module->variables, delete_var_, 0 );
    hash_free( module->variables );
}
