/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * hash.h - simple in-memory hashing routines
 */

#ifndef BOOST_JAM_HASH_H
#define BOOST_JAM_HASH_H

#include "object.h"

/*
 * An opaque struct representing an item in the hash table. The first element of
 * every struct stored in the table must be an OBJECT * which is treated as the
 * key.
 */
typedef struct hashdata HASHDATA;

/*
 * hashinit() - initialize a hash table, returning a handle.
 *
 * Parameters:
 *   datalen - item size
 *   name    - used for debugging
 */
struct hash * hashinit( int datalen, char const * name );

/*
 * hash_free() - free a hash table, given its handle
 */
void hash_free( struct hash * );
void hashdone( struct hash * );

/*
 * hashenumerate() - call f(i, data) on each item, i in the hash table. The
 * enumeration order is unspecified.
 */
void hashenumerate( struct hash *, void (* f)( void *, void * ), void * data );

/*
 * hash_insert() - insert a new item in a hash table, or return an existing one.
 *
 * Preconditions:
 *   - hp must be a hash table created by hashinit()
 *   - key must be an object created by object_new()
 *
 * Postconditions:
 *   - if the key does not already exist in the hash table, *found == 0 and the
 *     result will be a pointer to an uninitialized item. The key of the new
 *     item must be set to a value equal to key before any further operations on
 *     the hash table except hashdone().
 *   - if the key is present then *found == 1 and the result is a pointer to the
 *     existing record.
 */
HASHDATA * hash_insert( struct hash *, OBJECT * key, int * found );

/*
 * hash_find() - find a record in the table or NULL if none exists
 */
HASHDATA * hash_find( struct hash *, OBJECT * key );

struct hashstats {
    int count;
    int num_items;
    int tab_size;
    int item_size;
    int sets;
    int num_hashes;
};

void hashstats_init( struct hashstats * stats );
void hashstats_add( struct hashstats * stats, struct hash * );
void hashstats_print( struct hashstats * stats, char const * name );

#endif
