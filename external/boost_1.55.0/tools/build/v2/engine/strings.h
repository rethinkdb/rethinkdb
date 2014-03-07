/*
 * Copyright 2004. David Abrahams
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef STRINGS_DWA20011024_H
#define STRINGS_DWA20011024_H

#include <stddef.h>

typedef struct string
{
    char * value;
    unsigned long size;
    unsigned long capacity;
    char opt[ 32 ];
#ifndef NDEBUG
    char magic[ 4 ];
#endif
} string;

void string_new( string * );
void string_copy( string *, char const * );
void string_free( string * );
void string_append( string *, char const * );
void string_append_range( string *, char const *, char const * );
void string_push_back( string * s, char x );
void string_reserve( string *, size_t );
void string_truncate( string *, size_t );
void string_pop_back( string * );
char string_back( string * );
void string_unit_test();

#endif
