/*
 * Copyright 2003. Vladimir Prus
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "../mem.h"
#include "../native.h"
#include "../strings.h"
#include "../subst.h"

/*
rule split ( string separator )
{
    local result ;
    local s = $(string) ;

    local match = 1 ;
    while $(match)
    {
        match = [ MATCH ^(.*)($(separator))(.*) : $(s) ] ;
        if $(match)
        {
            match += "" ;  # in case 3rd item was empty - works around MATCH bug
            result = $(match[3]) $(result) ;
            s = $(match[1]) ;
        }
    }
    return $(s) $(result) ;
}
*/

LIST * regex_split( FRAME * frame, int flags )
{
    LIST * args = lol_get( frame->args, 0 );
    OBJECT * s;
    OBJECT * separator;
    regexp * re;
    const char * pos;
    LIST * result = L0;
    LISTITER iter = list_begin( args );
    s = list_item( iter );
    separator = list_item( list_next( iter ) );
    
    re = regex_compile( separator );

    pos = object_str( s );
    while ( regexec( re, pos ) )
    {
        result = list_push_back( result, object_new_range( pos, re->startp[ 0 ] - pos ) );
        pos = re->endp[ 0 ];
    }

    result = list_push_back( result, object_new( pos ) );

    return result;
}

/*
rule replace (
    string  # The string to modify.
    match  # The characters to replace.
    replacement  # The string to replace with.
    )
{
    local result = "" ;
    local parts = 1 ;
    while $(parts)
    {
        parts = [ MATCH ^(.*)($(match))(.*) : $(string) ] ;
        if $(parts)
        {
            parts += "" ;
            result = "$(replacement)$(parts[3])$(result)" ;
            string = $(parts[1]) ;
        }
    }
    string ?= "" ;
    result = "$(string)$(result)" ;
    return $(result) ;
}
*/

LIST * regex_replace( FRAME * frame, int flags )
{
    LIST * args = lol_get( frame->args, 0 );
    OBJECT * s;
    OBJECT * match;
    OBJECT * replacement;
    regexp * re;
    const char * pos;
    string buf[ 1 ];
    LIST * result;
    LISTITER iter = list_begin( args );
    s = list_item( iter );
    iter = list_next( iter );
    match = list_item( iter );
    iter = list_next( iter );
    replacement = list_item(iter );
    
    re = regex_compile( match );
    
    string_new( buf );

    pos = object_str( s );
    while ( regexec( re, pos ) )
    {
        string_append_range( buf, pos, re->startp[ 0 ] );
        string_append( buf, object_str( replacement ) );
        pos = re->endp[ 0 ];
    }
    string_append( buf, pos );

    result = list_new( object_new( buf->value ) );

    string_free( buf );

    return result;
}

/*
rule transform ( list * : pattern : indices * )
{
    indices ?= 1 ;
    local result ;
    for local e in $(list)
    {
        local m = [ MATCH $(pattern) : $(e) ] ;
        if $(m)
        {
            result += $(m[$(indices)]) ;
        }
    }
    return $(result) ;
}
*/

LIST * regex_transform( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LIST * const pattern = lol_get( frame->args, 1 );
    LIST * const indices_list = lol_get( frame->args, 2 );
    int * indices = 0;
    int size;
    LIST * result = L0;

    if ( !list_empty( indices_list ) )
    {
        int * p;
        LISTITER iter = list_begin( indices_list );
        LISTITER const end = list_end( indices_list );
        size = list_length( indices_list );
        indices = (int *)BJAM_MALLOC( size * sizeof( int ) );
        for ( p = indices; iter != end; iter = list_next( iter ) )
            *p++ = atoi( object_str( list_item( iter ) ) );
    }
    else
    {
        size = 1;
        indices = (int *)BJAM_MALLOC( sizeof( int ) );
        *indices = 1;
    }

    {
        /* Result is cached and intentionally never freed */
        regexp * const re = regex_compile( list_front( pattern ) );

        LISTITER iter = list_begin( l );
        LISTITER const end = list_end( l );

        string buf[ 1 ];
        string_new( buf );

        for ( ; iter != end; iter = list_next( iter ) )
        {
            if ( regexec( re, object_str( list_item( iter ) ) ) )
            {
                int i = 0;
                for ( ; i < size; ++i )
                {
                    int const index = indices[ i ];
                    /* Skip empty submatches. Not sure it is right in all cases,
                     * but surely is right for the case for which this routine
                     * is optimized -- header scanning.
                     */
                    if ( re->startp[ index ] != re->endp[ index ] )
                    {
                        string_append_range( buf, re->startp[ index ],
                            re->endp[ index ] );
                        result = list_push_back( result, object_new( buf->value
                            ) );
                        string_truncate( buf, 0 );
                    }
                }
            }
        }
        string_free( buf );
    }

    BJAM_FREE( indices );
    return result;
}


void init_regex()
{
    {
        char const * args[] = { "string", "separator", 0  };
        declare_native_rule( "regex", "split", args, regex_split, 1 );
    }
    {
        char const * args[] = { "string", "match", "replacement", 0  };
        declare_native_rule( "regex", "replace", args, regex_replace, 1 );
    }
    {
        char const * args[] = { "list", "*", ":", "pattern", ":", "indices", "*", 0 };
        declare_native_rule( "regex", "transform", args, regex_transform, 2 );
    }
}
