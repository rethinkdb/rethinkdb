/*
 * Copyright 2011 Steven Watanabe
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * object.h - object manipulation routines
 */

#ifndef BOOST_JAM_OBJECT_H
#define BOOST_JAM_OBJECT_H

typedef struct _object OBJECT;

OBJECT * object_new( char const * const );
OBJECT * object_new_range( char const * const, int const size );
void object_done( void );

#if defined(NDEBUG) && !defined(BJAM_NO_MEM_CACHE)

struct hash_header
{
    unsigned int hash;
    struct hash_item * next;
};

#define object_str( obj ) ((char const *)(obj))
#define object_copy( obj ) (obj)
#define object_free( obj ) ((void)0)
#define object_equal( lhs, rhs ) ((lhs) == (rhs))
#define object_hash( obj ) (((struct hash_header *)((char *)(obj) - sizeof(struct hash_header)))->hash)

#else

char const * object_str  ( OBJECT * );
OBJECT *     object_copy ( OBJECT * );
void         object_free ( OBJECT * );
int          object_equal( OBJECT *, OBJECT * );
unsigned int object_hash ( OBJECT * );

#endif

#endif
