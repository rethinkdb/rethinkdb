/*
 * Copyright 1993, 1995 Christopher Seiwald.
 * Copyright 2011 Steven Watanabe
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * object.c - object manipulation routines
 *
 * External functions:
 *    object_new()       - create an object from a string
 *    object_new_range() - create an object from a string of given length
 *    object_copy()      - return a copy of an object
 *    object_free()      - free an object
 *    object_str()       - get the string value of an object
 *    object_done()      - free string tables
 *
 * This implementation builds a hash table of all strings, so that multiple
 * calls of object_new() on the same string allocate memory for the string once.
 * Strings are never actually freed.
 */

#include "jam.h"
#include "object.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>


#define OBJECT_MAGIC 0xa762e0e3u

#ifndef object_copy

struct hash_header
{
#ifndef NDEBUG
    unsigned int magic;
#endif
    unsigned int hash;
    struct hash_item * next;
};

#endif

struct hash_item
{
    struct hash_header header;
    char data[ 1 ];
};

#define ALLOC_ALIGNMENT (sizeof(struct hash_item) - sizeof(struct hash_header))

typedef struct string_set
{
    unsigned int num;
    unsigned int size;
    struct hash_item * * data;
} string_set;

static string_set strhash;
static int strtotal = 0;
static int strcount_in = 0;
static int strcount_out = 0;


/*
 * Immortal string allocator implementation speeds string allocation and cuts
 * down on internal fragmentation.
 */

#define STRING_BLOCK 4096
typedef struct strblock
{
    struct strblock * next;
    char data[ STRING_BLOCK ];
} strblock;

static strblock * strblock_chain = 0;

/* Storage remaining in the current strblock */
static char * storage_start = 0;
static char * storage_finish = 0;


/*
 * allocate() - Allocate n bytes of immortal string storage.
 */

static char * allocate( size_t n )
{
#ifdef BJAM_NEWSTR_NO_ALLOCATE
    return (char *)BJAM_MALLOC( n );
#else
    /* See if we can grab storage from an existing block. */
    size_t remaining = storage_finish - storage_start;
    n = ( ( n + ALLOC_ALIGNMENT - 1 ) / ALLOC_ALIGNMENT ) * ALLOC_ALIGNMENT;
    if ( remaining >= n )
    {
        char * result = storage_start;
        storage_start += n;
        return result;
    }
    else  /* Must allocate a new block. */
    {
        strblock * new_block;
        size_t nalloc = n;
        if ( nalloc < STRING_BLOCK )
            nalloc = STRING_BLOCK;

        /* Allocate a new block and link into the chain. */
        new_block = (strblock *)BJAM_MALLOC( offsetof( strblock, data[ 0 ] ) +
            nalloc * sizeof( new_block->data[ 0 ] ) );
        if ( new_block == 0 )
            return 0;
        new_block->next = strblock_chain;
        strblock_chain = new_block;

        /* Take future allocations out of the larger remaining space. */
        if ( remaining < nalloc - n )
        {
            storage_start = new_block->data + n;
            storage_finish = new_block->data + nalloc;
        }
        return new_block->data;
    }
#endif
}


static unsigned int hash_keyval( char const * key, int const size )
{
    unsigned int const magic = 2147059363;
    unsigned int hash = 0;

    unsigned int i;
    for ( i = 0; i < size / sizeof( unsigned int ); ++i )
    {
        unsigned int val;
        memcpy( &val, key, sizeof( unsigned int ) );
        hash = hash * magic + val;
        key += sizeof( unsigned int );
    }

    {
        unsigned int val = 0;
        memcpy( &val, key, size % sizeof( unsigned int ) );
        hash = hash * magic + val;
    }

    return hash + ( hash >> 17 );
}


static void string_set_init( string_set * set )
{
    set->size = 0;
    set->num = 4;
    set->data = (struct hash_item * *)BJAM_MALLOC( set->num * sizeof( struct hash_item * ) );
    memset( set->data, 0, set->num * sizeof( struct hash_item * ) );
}


static void string_set_done( string_set * set )
{
    BJAM_FREE( set->data );
}


static void string_set_resize( string_set * set )
{
    unsigned i;
    string_set new_set;
    new_set.num = set->num * 2;
    new_set.size = set->size;
    new_set.data = (struct hash_item * *)BJAM_MALLOC( sizeof( struct hash_item *
        ) * new_set.num );
    memset( new_set.data, 0, sizeof( struct hash_item * ) * new_set.num );
    for ( i = 0; i < set->num; ++i )
    {
        while ( set->data[ i ] )
        {
            struct hash_item * temp = set->data[ i ];
            unsigned pos = temp->header.hash % new_set.num;
            set->data[ i ] = temp->header.next;
            temp->header.next = new_set.data[ pos ];
            new_set.data[ pos ] = temp;
        }
    }
    BJAM_FREE( set->data );
    *set = new_set;
}


static char const * string_set_insert( string_set * set, char const * string,
    int const size )
{
    unsigned hash = hash_keyval( string, size );
    unsigned pos = hash % set->num;

    struct hash_item * result;

    for ( result = set->data[ pos ]; result; result = result->header.next )
        if ( !strncmp( result->data, string, size ) && !result->data[ size ] )
            return result->data;

    if ( set->size >= set->num )
    {
        string_set_resize( set );
        pos = hash % set->num;
    }

    result = (struct hash_item *)allocate( sizeof( struct hash_header ) + size +
        1 );
    result->header.hash = hash;
    result->header.next = set->data[ pos ];
#ifndef NDEBUG
    result->header.magic = OBJECT_MAGIC;
#endif
    memcpy( result->data, string, size );
    result->data[ size ] = '\0';
    assert( hash_keyval( result->data, size ) == result->header.hash );
    set->data[ pos ] = result;
    strtotal += size + 1;
    ++set->size;

    return result->data;
}


static struct hash_item * object_get_item( OBJECT * obj )
{
    return (struct hash_item *)( (char *)obj - offsetof( struct hash_item, data
        ) );
}


static void object_validate( OBJECT * obj )
{
    assert( obj );
    assert( object_get_item( obj )->header.magic == OBJECT_MAGIC );
}


/*
 * object_new_range() - create an object from a string of given length
 */

OBJECT * object_new_range( char const * const string, int const size )
{
    ++strcount_in;

#ifdef BJAM_NO_MEM_CACHE
    {
        struct hash_item * const m = (struct hash_item *)BJAM_MALLOC( sizeof(
            struct hash_header ) + size + 1 );
        strtotal += size + 1;
        memcpy( m->data, string, size );
        m->data[ size ] = '\0';
        m->header.magic = OBJECT_MAGIC;
        return (OBJECT *)m->data;
    }
#else
    if ( !strhash.data )
        string_set_init( &strhash );
    return (OBJECT *)string_set_insert( &strhash, string, size );
#endif
}


/*
 * object_new() - create an object from a string
 */

OBJECT * object_new( char const * const string )
{
    return object_new_range( string, strlen( string ) );
}


#ifndef object_copy

/*
 * object_copy() - return a copy of an object
 */

OBJECT * object_copy( OBJECT * obj )
{
    object_validate( obj );
#ifdef BJAM_NO_MEM_CACHE
    return object_new( object_str( obj ) );
#else
    ++strcount_in;
    return obj;
#endif
}


/*
 * object_free() - free an object
 */

void object_free( OBJECT * obj )
{
    object_validate( obj );
#ifdef BJAM_NO_MEM_CACHE
    BJAM_FREE( object_get_item( obj ) );
#endif
    ++strcount_out;
}


/*
 * object_str() - return the OBJECT's internal C string
 */

char const * object_str( OBJECT * obj )
{
    object_validate( obj );
    return (char const *)obj;
}


/*
 * object_equal() - compare two objects
 */

int object_equal( OBJECT * lhs, OBJECT * rhs )
{
    object_validate( lhs );
    object_validate( rhs );
#ifdef BJAM_NO_MEM_CACHE
    return !strcmp( object_str( lhs ), object_str( rhs ) );
#else
    assert( ( lhs == rhs ) == !strcmp( object_str( lhs ), object_str( rhs ) ) );
    return lhs == rhs;
#endif
}


/*
 * object_hash() - returns the hash value of an object
 */

unsigned int object_hash( OBJECT * obj )
{
    object_validate( obj );
#ifdef BJAM_NO_MEM_CACHE
    return hash_keyval( object_str( obj ), strlen( object_str( obj ) ) );
#else
    return object_get_item( obj )->header.hash;
#endif
}

#endif

/*
 * object_done() - free string tables.
 */

void object_done()
{
#ifdef BJAM_NEWSTR_NO_ALLOCATE
    unsigned i;
    for ( i = 0; i < strhash.num; ++i )
    {
        while ( strhash.data[ i ] )
        {
            struct hash_item * item = strhash.data[ i ];
            strhash.data[ i ] = item->header.next;
            BJAM_FREE( item );
        }
    }
#else
    /* Reclaim string blocks. */
    while ( strblock_chain )
    {
        strblock * const n = strblock_chain->next;
        BJAM_FREE( strblock_chain );
        strblock_chain = n;
    }
#endif

    string_set_done( &strhash );

    if ( DEBUG_MEM )
    {
        printf( "%dK in strings\n", strtotal / 1024 );
        if ( strcount_in != strcount_out )
            printf( "--- %d strings of %d dangling\n", strcount_in -
                strcount_out, strcount_in );
    }
}
