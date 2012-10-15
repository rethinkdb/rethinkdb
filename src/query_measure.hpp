#ifndef QUERY_MEASURE_HPP_
#define QUERY_MEASURE_HPP_

// Utilities for measuring query latency. Define RDB_QUERY_MEASURE to 1 to turn measurement on.
// You may have to uncomment certain logRQM lines in order to see actual output.  The
// measurement is primitive right now -- you can measure particular functions, but you can't
// make a grand measurement for a particular query.  You're welcome to add such functionality.

#define RDB_QUERY_MEASURE 0

#if RDB_QUERY_MEASURE
#include "errors.hpp"
#include "logger.hpp"
#include "utils.hpp"

#define TICKVAR(x) UNUSED const ticks_t x = get_ticks()
#define DTICKVAR(x) UNUSED ticks_t x = 0;
#define ATICKVAR(x) ((x) = get_ticks())
#define SETPATH(x, y) ((x) = (y))
#define logRQM(...) logINF(__VA_ARGS__)
#else
#define TICKVAR(x) (void)0
#define DTICKVAR(x) (void)0
#define ATICKVAR(x) (void)0
#define SETPATH(x, y) (void)0
#define logRQM(...) (void)0
#endif  // RDB_QUERY_MEASURE

#endif  // QUERY_MEASURE_HPP_
