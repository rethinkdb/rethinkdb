#ifndef PERFMON_COLLECT_HPP_
#define PERFMON_COLLECT_HPP_

#include "perfmon/core.hpp"

/* `perfmon_get_stats()` collects all the stats about the server and puts them
 * into the given `perfmon_result_t` object. It must be run in a coroutine and it
 * blocks until it is done.
 */
void perfmon_get_stats(perfmon_result_t *dest);

#endif  // PERFMON_COLLECT_HPP_
