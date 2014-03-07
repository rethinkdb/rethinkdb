/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*  This file is ALSO:
 *  Copyright 2001-2004 David Abrahams.
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * lists.h - the LIST structure and routines to manipulate them
 *
 * The whole of jam relies on lists of objects as a datatype. This module, in
 * conjunction with object.c, handles these relatively efficiently.
 *
 * Structures defined:
 *
 *  LIST - list of OBJECTs
 *  LOL - list of LISTs
 *
 * External routines:
 *
 *  list_append() - append a list onto another one, returning total
 *  list_new() - tack an object onto the end of a list of objects
 *  list_copy() - copy a whole list of objects
 *  list_sublist() - copy a subset of a list of objects
 *  list_free() - free a list of objects
 *  list_print() - print a list of objects to stdout
 *  list_length() - return the number of items in the list
 *
 *  lol_init() - initialize a LOL (list of lists)
 *  lol_add() - append a LIST onto an LOL
 *  lol_free() - free the LOL and its LISTs
 *  lol_get() - return one of the LISTs in the LOL
 *  lol_print() - debug print LISTS separated by ":"
 */

#ifndef LISTS_DWA20011022_H
#define LISTS_DWA20011022_H

#include "object.h"

#ifdef HAVE_PYTHON
# include <Python.h>
#endif

/*
 * LIST - list of strings
 */

typedef struct _list {
    union {
        int size;
        struct _list * next;
        OBJECT * align;
    } impl;
} LIST;

typedef OBJECT * * LISTITER;

/*
 * LOL - list of LISTs
 */

#define LOL_MAX 19
typedef struct _lol {
    int count;
    LIST * list[ LOL_MAX ];
} LOL;

LIST * list_new( OBJECT * value );
LIST * list_append( LIST * destination, LIST * source );
LIST * list_copy( LIST * );
LIST * list_copy_range( LIST * destination, LISTITER first, LISTITER last );
void   list_free( LIST * head );
LIST * list_push_back( LIST * head, OBJECT * value );
void   list_print( LIST * );
int    list_length( LIST * );
LIST * list_sublist( LIST *, int start, int count );
LIST * list_pop_front( LIST * );
LIST * list_sort( LIST * );
LIST * list_unique( LIST * sorted_list );
int    list_in( LIST *, OBJECT * value );
LIST * list_reverse( LIST * );
int    list_cmp( LIST * lhs, LIST * rhs );
int    list_is_sublist( LIST * sub, LIST * l );
void   list_done();

LISTITER list_begin( LIST * );
LISTITER list_end( LIST * );
#define list_next( it ) ((it) + 1)
#define list_item( it ) (*(it))
#define list_empty( l ) ((l) == L0)
#define list_front( l ) list_item( list_begin( l ) )

#define L0 ((LIST *)0)

void   lol_add( LOL *, LIST * );
void   lol_init( LOL * );
void   lol_free( LOL * );
LIST * lol_get( LOL *, int i );
void   lol_print( LOL * );
void   lol_build( LOL *, char const * * elements );

#ifdef HAVE_PYTHON
PyObject * list_to_python( LIST * );
LIST * list_from_python( PyObject * );
#endif

#endif
