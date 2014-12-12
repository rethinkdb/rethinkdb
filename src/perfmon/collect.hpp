// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef PERFMON_COLLECT_HPP_
#define PERFMON_COLLECT_HPP_

#include "perfmon/core.hpp"

/* `perfmon_get_stats()` collects all the stats about the server and puts them
 * into the `ql::datum_t` object. It must be run in a coroutine and it
 * blocks until it is done.
 */
ql::datum_t perfmon_get_stats();

#endif  // PERFMON_COLLECT_HPP_
