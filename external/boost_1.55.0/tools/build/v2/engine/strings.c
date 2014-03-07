/* Copyright David Abrahams 2004. Distributed under the Boost */
/* Software License, Version 1.0. (See accompanying */
/* file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt) */

#include "jam.h"
#include "strings.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


#ifndef NDEBUG
# define JAM_STRING_MAGIC ((char)0xcf)
# define JAM_STRING_MAGIC_SIZE 4
static void assert_invariants( string * self )
{
    int i;

    if ( self->value == 0 )
    {
        assert( self->size == 0 );
        assert( self->capacity == 0 );
        assert( self->opt[ 0 ] == 0 );
        return;
    }

    assert( self->size < self->capacity );
    assert( ( self->capacity <= sizeof( self->opt ) ) == ( self->value == self->opt ) );
    assert( self->value[ self->size ] == 0 );
    /* String objects modified manually after construction to contain embedded
     * '\0' characters are considered structurally valid.
     */
    assert( strlen( self->value ) <= self->size );

    for ( i = 0; i < 4; ++i )
    {
        assert( self->magic[ i ] == JAM_STRING_MAGIC );
        assert( self->value[ self->capacity + i ] == JAM_STRING_MAGIC );
    }
}
#else
# define JAM_STRING_MAGIC_SIZE 0
# define assert_invariants(x) do {} while (0)
#endif


void string_new( string * s )
{
    s->value = s->opt;
    s->size = 0;
    s->capacity = sizeof( s->opt );
    s->opt[ 0 ] = 0;
#ifndef NDEBUG
    memset( s->magic, JAM_STRING_MAGIC, sizeof( s->magic ) );
#endif
    assert_invariants( s );
}


void string_free( string * s )
{
    assert_invariants( s );
    if ( s->value != s->opt )
        BJAM_FREE( s->value );
    string_new( s );
}


static void string_reserve_internal( string * self, size_t capacity )
{
    if ( self->value == self->opt )
    {
        self->value = (char *)BJAM_MALLOC_ATOMIC( capacity +
            JAM_STRING_MAGIC_SIZE );
        self->value[ 0 ] = 0;
        strncat( self->value, self->opt, sizeof(self->opt) );
        assert( strlen( self->value ) <= self->capacity && "Regression test" );
    }
    else
    {
        self->value = (char *)BJAM_REALLOC( self->value, capacity +
            JAM_STRING_MAGIC_SIZE );
    }
#ifndef NDEBUG
    memcpy( self->value + capacity, self->magic, JAM_STRING_MAGIC_SIZE );
#endif
    self->capacity = capacity;
}


void string_reserve( string * self, size_t capacity )
{
    assert_invariants( self );
    if ( capacity <= self->capacity )
        return;
    string_reserve_internal( self, capacity );
    assert_invariants( self );
}


static void extend_full( string * self, char const * start, char const * finish )
{
    size_t new_size = self->capacity + ( finish - start );
    size_t new_capacity = self->capacity;
    size_t old_size = self->capacity;
    while ( new_capacity < new_size + 1)
        new_capacity <<= 1;
    string_reserve_internal( self, new_capacity );
    memcpy( self->value + old_size, start, new_size - old_size );
    self->value[ new_size ] = 0;
    self->size = new_size;
}

static void maybe_reserve( string * self, size_t new_size )
{
    size_t capacity = self->capacity;
    if ( capacity <= new_size )
    {
        size_t new_capacity = capacity;
        while ( new_capacity <= new_size )
            new_capacity <<= 1;
        string_reserve_internal( self, new_capacity );
    }
}


void string_append( string * self, char const * rhs )
{
    size_t rhs_size = strlen( rhs );
    size_t new_size = self->size + rhs_size;
    assert_invariants( self );

    maybe_reserve( self, new_size );

    memcpy( self->value + self->size, rhs, rhs_size + 1 );
    self->size = new_size;

    assert_invariants( self );
}


void string_append_range( string * self, char const * start, char const * finish )
{
    size_t rhs_size = finish - start;
    size_t new_size = self->size + rhs_size;
    assert_invariants( self );

    maybe_reserve( self, new_size );

    memcpy( self->value + self->size, start, rhs_size );
    self->size = new_size;
    self->value[ new_size ] = 0;

    assert_invariants( self );
}


void string_copy( string * s, char const * rhs )
{
    string_new( s );
    string_append( s, rhs );
}

void string_truncate( string * self, size_t n )
{
    assert_invariants( self );
    assert( n <= self->capacity );
    self->value[ self->size = n ] = 0;
    assert_invariants( self );
}


void string_pop_back( string * self )
{
    string_truncate( self, self->size - 1 );
}


void string_push_back( string * self, char x )
{
    string_append_range( self, &x, &x + 1 );
}


char string_back( string * self )
{
    assert_invariants( self );
    return self->value[ self->size - 1 ];
}


#ifndef NDEBUG
void string_unit_test()
{
    {
        string s[ 1 ];
        int i;
        int const limit = sizeof( s->opt ) * 2 + 2;
        string_new( s );
        assert( s->value == s->opt );
        for ( i = 0; i < limit; ++i )
        {
            string_push_back( s, (char)( i + 1 ) );
            assert( s->size == i + 1 );
        }
        assert( s->size == limit );
        assert( s->value != s->opt );
        for ( i = 0; i < limit; ++i )
            assert( s->value[ i ] == (char)( i + 1 ) );
        string_free( s );
    }

    {
        char * const original = "  \n\t\v  Foo \r\n\v \tBar\n\n\r\r\t\n\v\t \t";
        string copy[ 1 ];
        string_copy( copy, original );
        assert( !strcmp( copy->value, original ) );
        assert( copy->size == strlen( original ) );
        string_free( copy );
    }
}
#endif
