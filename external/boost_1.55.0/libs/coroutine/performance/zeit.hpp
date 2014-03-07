
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ZEIT_H
#define ZEIT_H

#include <time.h>

#include <algorithm>
#include <numeric>
#include <cstddef>
#include <vector>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>

typedef boost::uint64_t zeit_t;

inline
zeit_t zeit()
{
    timespec t;
    ::clock_gettime( CLOCK_PROCESS_CPUTIME_ID, & t);
    return t.tv_sec * 1000000000 + t.tv_nsec;
}

struct measure_zeit
{
    zeit_t operator()()
    {
        zeit_t start( zeit() );
        return zeit() - start;
    }
};

inline
zeit_t overhead_zeit()
{
    std::size_t iterations( 10);
    std::vector< zeit_t >  overhead( iterations, 0);
    for ( std::size_t i( 0); i < iterations; ++i)
        std::generate(
            overhead.begin(), overhead.end(),
            measure_zeit() );
    BOOST_ASSERT( overhead.begin() != overhead.end() );
    return std::accumulate( overhead.begin(), overhead.end(), 0) / iterations;
}

#endif // ZEIT_H
