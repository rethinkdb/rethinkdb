/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * timestamp.h - get the timestamp of a file or archive member
 */

#ifndef TIMESTAMP_H_SW_2011_11_18
#define TIMESTAMP_H_SW_2011_11_18

#include "object.h"

#ifdef OS_NT
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include <time.h>

typedef struct timestamp
{
    time_t secs;
    int nsecs;
} timestamp;

void timestamp_clear( timestamp * const );
int timestamp_cmp( timestamp const * const lhs, timestamp const * const rhs );
void timestamp_copy( timestamp * const target, timestamp const * const source );
void timestamp_current( timestamp * const );
int timestamp_empty( timestamp const * const );
void timestamp_from_path( timestamp * const, OBJECT * const path );
void timestamp_init( timestamp * const, time_t const secs, int const nsecs );
void timestamp_max( timestamp * const max, timestamp const * const lhs,
    timestamp const * const rhs );
char const * timestamp_str( timestamp const * const );
char const * timestamp_timestr( timestamp const * const );

#ifdef OS_NT
void timestamp_from_filetime( timestamp * const, FILETIME const * const );
#endif

void timestamp_done();

#endif
