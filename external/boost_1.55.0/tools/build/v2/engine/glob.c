/*
 * Copyright 1994 Christopher Seiwald.  All rights reserved.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * glob.c - match a string against a simple pattern
 *
 * Understands the following patterns:
 *
 *  *   any number of characters
 *  ?   any single character
 *  [a-z]   any single character in the range a-z
 *  [^a-z]  any single character not in the range a-z
 *  \x  match x
 *
 * External functions:
 *
 *  glob() - match a string against a simple pattern
 *
 * Internal functions:
 *
 *  globchars() - build a bitlist to check for character group match
 */

# include "jam.h"

# define CHECK_BIT( tab, bit ) ( tab[ (bit)/8 ] & (1<<( (bit)%8 )) )
# define BITLISTSIZE 16 /* bytes used for [chars] in compiled expr */

static void globchars( const char * s, const char * e, char * b );


/*
 * glob() - match a string against a simple pattern.
 */

int glob( const char * c, const char * s )
{
    char   bitlist[ BITLISTSIZE ];
    const char * here;

    for ( ; ; )
    switch ( *c++ )
    {
    case '\0':
        return *s ? -1 : 0;

    case '?':
        if ( !*s++ )
            return 1;
        break;

    case '[':
        /* Scan for matching ]. */

        here = c;
        do if ( !*c++ ) return 1;
        while ( ( here == c ) || ( *c != ']' ) );
        ++c;

        /* Build character class bitlist. */

        globchars( here, c, bitlist );

        if ( !CHECK_BIT( bitlist, *(const unsigned char *)s ) )
            return 1;
        ++s;
        break;

    case '*':
        here = s;

        while ( *s )
            ++s;

        /* Try to match the rest of the pattern in a recursive */
        /* call.  If the match fails we'll back up chars, retrying. */

        while ( s != here )
        {
            int r;

            /* A fast path for the last token in a pattern. */
            r = *c ? glob( c, s ) : *s ? -1 : 0;

            if ( !r )
                return 0;
            if ( r < 0 )
                return 1;
            --s;
        }
        break;

    case '\\':
        /* Force literal match of next char. */
        if ( !*c || ( *s++ != *c++ ) )
            return 1;
        break;

    default:
        if ( *s++ != c[ -1 ] )
            return 1;
        break;
    }
}


/*
 * globchars() - build a bitlist to check for character group match.
 */

static void globchars( const char * s,  const char * e, char * b )
{
    int neg = 0;

    memset( b, '\0', BITLISTSIZE  );

    if ( *s == '^' )
    {
        ++neg;
        ++s;
    }

    while ( s < e )
    {
        int c;

        if ( ( s + 2 < e ) && ( s[1] == '-' ) )
        {
            for ( c = s[0]; c <= s[2]; ++c )
                b[ c/8 ] |= ( 1 << ( c % 8 ) );
            s += 3;
        }
        else
        {
            c = *s++;
            b[ c/8 ] |= ( 1 << ( c % 8 ) );
        }
    }

    if ( neg )
    {
        int i;
        for ( i = 0; i < BITLISTSIZE; ++i )
            b[ i ] ^= 0377;
    }

    /* Do not include \0 in either $[chars] or $[^chars]. */
    b[0] &= 0376;
}
