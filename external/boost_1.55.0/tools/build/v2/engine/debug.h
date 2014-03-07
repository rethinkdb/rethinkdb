/*
 * Copyright 2005. Rene Rivera
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BJAM_DEBUG_H
#define BJAM_DEBUG_H

#include "constants.h"
#include "object.h"
#include <time.h>


typedef struct profile_info
{
    /* name of rule being called */
    OBJECT * name;
    /* cumulative time spent in rule */
    clock_t cumulative;
    /* time spent in rule proper */
    clock_t net;
    /* number of time rule was entered */
    unsigned long num_entries;
    /* number of the times this function is present in stack */
    unsigned long stack_count;
    /* bytes of memory allocated by the call */
    unsigned long memory;
} profile_info;

typedef struct profile_frame
{
    /* permanent storage where data accumulates */
    profile_info * info;
    /* overhead for profiling in this call */
    clock_t overhead;
    /* time of last entry to rule */
    clock_t entry_time;
    /* stack frame of caller */
    struct profile_frame * caller;
    /* time spent in subrules */
    clock_t subrules;
} profile_frame;

profile_frame * profile_init( OBJECT * rulename, profile_frame * );
void profile_enter( OBJECT * rulename, profile_frame * );
void profile_memory( long mem );
void profile_exit( profile_frame * );
void profile_dump();

#define PROFILE_ENTER( scope ) profile_frame PROF_ ## scope, *PROF_ ## scope ## _p = profile_init( constant_ ## scope, &PROF_ ## scope )
#define PROFILE_EXIT( scope ) profile_exit( PROF_ ## scope ## _p )

#endif
