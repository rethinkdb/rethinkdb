#include "jam.h"
#include "subst.h"

#include "builtins.h"
#include "frames.h"
#include "hash.h"
#include "lists.h"

#include <stddef.h>


typedef struct regex_entry
{
    OBJECT * pattern;
    regexp * regex;
} regex_entry;

static struct hash * regex_hash;


regexp * regex_compile( OBJECT * pattern )
{
    int found;
    regex_entry * e ;

    if ( !regex_hash )
        regex_hash = hashinit( sizeof( regex_entry ), "regex" );

    e = (regex_entry *)hash_insert( regex_hash, pattern, &found );
    if ( !found )
    {
        e->pattern = object_copy( pattern );
        e->regex = regcomp( (char *)pattern );
    }

    return e->regex;
}


LIST * builtin_subst( FRAME * frame, int flags )
{
    LIST * result = L0;
    LIST * const arg1 = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( arg1 );
    LISTITER const end = list_end( arg1 );

    if ( iter != end && list_next( iter ) != end && list_next( list_next( iter )
        ) != end )
    {
        char const * const source = object_str( list_item( iter ) );
        OBJECT * const pattern = list_item( list_next( iter ) );
        regexp * const repat = regex_compile( pattern );

        if ( regexec( repat, (char *)source) )
        {
            LISTITER subst = list_next( iter );

            while ( ( subst = list_next( subst ) ) != end )
            {
#define BUFLEN 4096
                char buf[ BUFLEN + 1 ];
                char const * in = object_str( list_item( subst ) );
                char * out = buf;

                for ( ; *in && out < buf + BUFLEN; ++in )
                {
                    if ( *in == '\\' || *in == '$' )
                    {
                        ++in;
                        if ( *in == 0 )
                            break;
                        if ( *in >= '0' && *in <= '9' )
                        {
                            unsigned int const n = *in - '0';
                            size_t const srclen = repat->endp[ n ] -
                                repat->startp[ n ];
                            size_t const remaining = buf + BUFLEN - out;
                            size_t const len = srclen < remaining
                                ? srclen
                                : remaining;
                            memcpy( out, repat->startp[ n ], len );
                            out += len;
                            continue;
                        }
                        /* fall through and copy the next character */
                    }
                    *out++ = *in;
                }
                *out = 0;

                result = list_push_back( result, object_new( buf ) );
#undef BUFLEN
            }
        }
    }

    return result;
}


static void free_regex( void * xregex, void * data )
{
    regex_entry * const regex = (regex_entry *)xregex;
    object_free( regex->pattern );
    BJAM_FREE( regex->regex );
}


void regex_done()
{
    if ( regex_hash )
    {
        hashenumerate( regex_hash, free_regex, (void *)0 );
        hashdone( regex_hash );
    }
}
