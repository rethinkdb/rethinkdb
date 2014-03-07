/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * hash.c - simple in-memory hashing routines
 *
 * External routines:
 *     hashinit() - initialize a hash table, returning a handle
 *     hashitem() - find a record in the table, and optionally enter a new one
 *     hashdone() - free a hash table, given its handle
 *
 * Internal routines:
 *     hashrehash() - resize and rebuild hp->tab, the hash table
 */

#include "jam.h"
#include "hash.h"

#include "compile.h"

#include <assert.h>

/* */
#define HASH_DEBUG_PROFILE 1
/* */

/* Header attached to all hash table data items. */

typedef struct item ITEM;
struct item
{
    ITEM * next;
};

#define MAX_LISTS 32

struct hash
{
    /*
     * the hash table, just an array of item pointers
     */
    struct
    {
        int nel;
        ITEM * * base;
    } tab;

    int bloat;  /* tab.nel / items.nel */
    int inel;   /* initial number of elements */

    /*
     * the array of records, maintained by these routines - essentially a
     * microallocator
     */
    struct
    {
        int more;     /* how many more ITEMs fit in lists[ list ] */
        ITEM * free;  /* free list of items */
        char * next;  /* where to put more ITEMs in lists[ list ] */
        int size;     /* sizeof( ITEM ) + aligned datalen */
        int nel;      /* total ITEMs held by all lists[] */
        int list;     /* index into lists[] */

        struct
        {
            int nel;      /* total ITEMs held by this list */
            char * base;  /* base of ITEMs array */
        } lists[ MAX_LISTS ];
    } items;

    char const * name;  /* just for hashstats() */
};

static void hashrehash( struct hash * );
static void hashstat( struct hash * );

static unsigned int hash_keyval( OBJECT * key )
{
    return object_hash( key );
}

#define hash_bucket(hp, keyval) ((hp)->tab.base + ((keyval) % (hp)->tab.nel))

#define hash_data_key(data) (*(OBJECT * *)(data))
#define hash_item_data(item) ((HASHDATA *)((char *)item + sizeof(ITEM)))
#define hash_item_key(item) (hash_data_key(hash_item_data(item)))


#define ALIGNED(x) ((x + sizeof(ITEM) - 1) & ~(sizeof(ITEM) - 1))

/*
 * hashinit() - initialize a hash table, returning a handle
 */

struct hash * hashinit( int datalen, char const * name )
{
    struct hash * hp = (struct hash *)BJAM_MALLOC( sizeof( *hp ) );

    hp->bloat = 3;
    hp->tab.nel = 0;
    hp->tab.base = 0;
    hp->items.more = 0;
    hp->items.free = 0;
    hp->items.size = sizeof( ITEM ) + ALIGNED( datalen );
    hp->items.list = -1;
    hp->items.nel = 0;
    hp->inel = 11;  /* 47 */
    hp->name = name;

    return hp;
}


/*
 * hash_search() - Find the hash item for the given data.
 *
 * Returns a pointer to a hashed item with the given key. If given a 'previous'
 * pointer, makes it point to the item prior to the found item in the same
 * bucket or to 0 if our item is the first item in its bucket.
 */

static ITEM * hash_search( struct hash * hp, unsigned int keyval,
    OBJECT * keydata, ITEM * * previous )
{
    ITEM * i = *hash_bucket( hp, keyval );
    ITEM * p = 0;
    for ( ; i; i = i->next )
    {
        if ( object_equal( hash_item_key( i ), keydata ) )
        {
            if ( previous )
                *previous = p;
            return i;
        }
        p = i;
    }
    return 0;
}


/*
 * hash_insert() - insert a record in the table or return the existing one
 */

HASHDATA * hash_insert( struct hash * hp, OBJECT * key, int * found )
{
    ITEM * i;
    unsigned int keyval = hash_keyval( key );

    #ifdef HASH_DEBUG_PROFILE
    profile_frame prof[ 1 ];
    if ( DEBUG_PROFILE )
        profile_enter( 0, prof );
    #endif

    if ( !hp->items.more )
        hashrehash( hp );

    i = hash_search( hp, keyval, key, 0 );
    if ( i )
        *found = 1;
    else
    {
        ITEM * * base = hash_bucket( hp, keyval );

        /* Try to grab one from the free list. */
        if ( hp->items.free )
        {
            i = hp->items.free;
            hp->items.free = i->next;
            assert( !hash_item_key( i ) );
        }
        else
        {
            i = (ITEM *)hp->items.next;
            hp->items.next += hp->items.size;
        }
        --hp->items.more;
        i->next = *base;
        *base = i;
        *found = 0;
    }

    #ifdef HASH_DEBUG_PROFILE
    if ( DEBUG_PROFILE )
        profile_exit( prof );
    #endif

    return hash_item_data( i );
}


/*
 * hash_find() - find a record in the table or NULL if none exists
 */

HASHDATA * hash_find( struct hash * hp, OBJECT * key )
{
    ITEM * i;
    unsigned int keyval = hash_keyval( key );

    #ifdef HASH_DEBUG_PROFILE
    profile_frame prof[ 1 ];
    if ( DEBUG_PROFILE )
        profile_enter( 0, prof );
    #endif

    if ( !hp->items.nel )
    {
        #ifdef HASH_DEBUG_PROFILE
        if ( DEBUG_PROFILE )
            profile_exit( prof );
        #endif
        return 0;
    }

    i = hash_search( hp, keyval, key, 0 );

    #ifdef HASH_DEBUG_PROFILE
    if ( DEBUG_PROFILE )
        profile_exit( prof );
    #endif

    return i ? hash_item_data( i ) : 0;
}


/*
 * hashrehash() - resize and rebuild hp->tab, the hash table
 */

static void hashrehash( struct hash * hp )
{
    int i = ++hp->items.list;
    hp->items.more = i ? 2 * hp->items.nel : hp->inel;
    hp->items.next = (char *)BJAM_MALLOC( hp->items.more * hp->items.size );
    hp->items.free = 0;

    hp->items.lists[ i ].nel = hp->items.more;
    hp->items.lists[ i ].base = hp->items.next;
    hp->items.nel += hp->items.more;

    if ( hp->tab.base )
        BJAM_FREE( (char *)hp->tab.base );

    hp->tab.nel = hp->items.nel * hp->bloat;
    hp->tab.base = (ITEM * *)BJAM_MALLOC( hp->tab.nel * sizeof( ITEM * * ) );

    memset( (char *)hp->tab.base, '\0', hp->tab.nel * sizeof( ITEM * ) );

    for ( i = 0; i < hp->items.list; ++i )
    {
        int nel = hp->items.lists[ i ].nel;
        char * next = hp->items.lists[ i ].base;

        for ( ; nel--; next += hp->items.size )
        {
            ITEM * i = (ITEM *)next;
            ITEM * * ip = hp->tab.base + object_hash( hash_item_key( i ) ) %
                hp->tab.nel;
            /* code currently assumes rehashing only when there are no free
             * items
             */
            assert( hash_item_key( i ) );

            i->next = *ip;
            *ip = i;
        }
    }
}


void hashenumerate( struct hash * hp, void (* f)( void *, void * ), void * data
    )
{
    int i;
    for ( i = 0; i <= hp->items.list; ++i )
    {
        char * next = hp->items.lists[ i ].base;
        int nel = hp->items.lists[ i ].nel;
        if ( i == hp->items.list )
            nel -= hp->items.more;

        for ( ; nel--; next += hp->items.size )
        {
            ITEM * const i = (ITEM *)next;
            if ( hash_item_key( i ) != 0 )  /* Do not enumerate freed items. */
                f( hash_item_data( i ), data );
        }
    }
}


/*
 * hash_free() - free a hash table, given its handle
 */

void hash_free( struct hash * hp )
{
    int i;
    if ( !hp )
        return;
    if ( hp->tab.base )
        BJAM_FREE( (char *)hp->tab.base );
    for ( i = 0; i <= hp->items.list; ++i )
        BJAM_FREE( hp->items.lists[ i ].base );
    BJAM_FREE( (char *)hp );
}


static void hashstat( struct hash * hp )
{
    struct hashstats stats[ 1 ];
    hashstats_init( stats );
    hashstats_add( stats, hp );
    hashstats_print( stats, hp->name );
}


void hashstats_init( struct hashstats * stats )
{
    stats->count = 0;
    stats->num_items = 0;
    stats->tab_size = 0;
    stats->item_size = 0;
    stats->sets = 0;
    stats->num_hashes = 0;
}


void hashstats_add( struct hashstats * stats, struct hash * hp )
{
    if ( hp )
    {
        ITEM * * tab = hp->tab.base;
        int nel = hp->tab.nel;
        int count = 0;
        int sets = 0;
        int i;

        for ( i = 0; i < nel; ++i )
        {
            ITEM * item;
            int here = 0;
            for ( item = tab[ i ]; item; item = item->next )
                ++here;

            count += here;
            if ( here > 0 )
                ++sets;
        }

        stats->count += count;
        stats->sets += sets;
        stats->num_items += hp->items.nel;
        stats->tab_size += hp->tab.nel;
        stats->item_size = hp->items.size;
        ++stats->num_hashes;
    }
}


void hashstats_print( struct hashstats * stats, char const * name )
{
    printf( "%s table: %d+%d+%d (%dK+%luK+%luK) items+table+hash, %f density\n",
        name,
        stats->count,
        stats->num_items,
        stats->tab_size,
        stats->num_items * stats->item_size / 1024,
        (long unsigned)stats->tab_size * sizeof( ITEM * * ) / 1024,
        (long unsigned)stats->num_hashes * sizeof( struct hash ) / 1024,
        (float)stats->count / (float)stats->sets );
}


void hashdone( struct hash * hp )
{
    if ( !hp )
        return;
    if ( DEBUG_MEM || DEBUG_PROFILE )
        hashstat( hp );
    hash_free( hp );
}
