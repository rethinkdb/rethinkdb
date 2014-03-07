/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * hdrmacro.c - handle header files that define macros used in #include
 *              statements.
 *
 *  we look for lines like "#define MACRO  <....>" or '#define MACRO  "    "' in
 *  the target file. When found, we then phony up a rule invocation like:
 *
 *  $(HDRRULE) <target> : <resolved included files> ;
 *
 * External routines:
 *    headers1() - scan a target for "#include MACRO" lines and try to resolve
 *                 them when needed
 *
 * Internal routines:
 *    headers1() - using regexp, scan a file and build include LIST
 */

#include "jam.h"
#include "hdrmacro.h"

#include "compile.h"
#include "hash.h"
#include "lists.h"
#include "object.h"
#include "parse.h"
#include "rules.h"
#include "strings.h"
#include "subst.h"
#include "variable.h"


/* this type is used to store a dictionary of file header macros */
typedef struct header_macro
{
  OBJECT * symbol;
  OBJECT * filename;  /* we could maybe use a LIST here ?? */
} HEADER_MACRO;

static struct hash * header_macros_hash = 0;


/*
 * headers() - scan a target for include files and call HDRRULE
 */

#define MAXINC 10

void macro_headers( TARGET * t )
{
    static regexp * re = 0;
    FILE * f;
    char buf[ 1024 ];

    if ( DEBUG_HEADER )
        printf( "macro header scan for %s\n", object_str( t->name ) );

    /* This regexp is used to detect lines of the form
     * "#define  MACRO  <....>" or "#define  MACRO  "....."
     * in the header macro files.
     */
    if ( !re )
    {
        OBJECT * const re_str = object_new(
            "^[     ]*#[    ]*define[   ]*([A-Za-z][A-Za-z0-9_]*)[  ]*"
            "[<\"]([^\">]*)[\">].*$" );
        re = regex_compile( re_str );
        object_free( re_str );
    }

    if ( !( f = fopen( object_str( t->boundname ), "r" ) ) )
        return;

    while ( fgets( buf, sizeof( buf ), f ) )
    {
        HEADER_MACRO var;
        HEADER_MACRO * v = &var;

        if ( regexec( re, buf ) && re->startp[ 1 ] )
        {
            OBJECT * symbol;
            int found;
            /* we detected a line that looks like "#define  MACRO  filename */
            ( (char *)re->endp[ 1 ] )[ 0 ] = '\0';
            ( (char *)re->endp[ 2 ] )[ 0 ] = '\0';

            if ( DEBUG_HEADER )
                printf( "macro '%s' used to define filename '%s' in '%s'\n",
                    re->startp[ 1 ], re->startp[ 2 ], object_str( t->boundname )
                    );

            /* add macro definition to hash table */
            if ( !header_macros_hash )
                header_macros_hash = hashinit( sizeof( HEADER_MACRO ),
                    "hdrmacros" );

            symbol = object_new( re->startp[ 1 ] );
            v = (HEADER_MACRO *)hash_insert( header_macros_hash, symbol, &found
                );
            if ( !found )
            {
                v->symbol = symbol;
                v->filename = object_new( re->startp[ 2 ] );  /* never freed */
            }
            else
                object_free( symbol );
            /* XXXX: FOR NOW, WE IGNORE MULTIPLE MACRO DEFINITIONS !! */
            /*       WE MIGHT AS WELL USE A LIST TO STORE THEM..      */
        }
    }

    fclose( f );
}


OBJECT * macro_header_get( OBJECT * macro_name )
{
    HEADER_MACRO * v;
    if ( header_macros_hash && ( v = (HEADER_MACRO *)hash_find(
        header_macros_hash, macro_name ) ) )
    {
        if ( DEBUG_HEADER )
            printf( "### macro '%s' evaluated to '%s'\n", object_str( macro_name
                ), object_str( v->filename ) );
        return v->filename;
    }
    return 0;
}
