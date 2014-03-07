
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef CYCLE_I386_H
#define CYCLE_I386_H

#include <algorithm>
#include <numeric>
#include <cstddef>
#include <vector>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>

#define BOOST_CONTEXT_CYCLE

typedef boost::uint64_t cycle_t;

#if _MSC_VER
inline
cycle_t cycles()
{
    cycle_t c;
    __asm {
        cpuid 
        rdtsc
        mov dword ptr [c + 0], eax
        mov dword ptr [c + 4], edx
    }
    return c;
}
#elif defined(__GNUC__) || \
      defined(__INTEL_COMPILER) || defined(__ICC) || defined(_ECC) || defined(__ICL)
inline
cycle_t cycles()
{
    boost::uint32_t lo, hi;

    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"
        ::: "%eax", "%ebx", "%ecx", "%edx"
    );
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi) );
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"
        ::: "%eax", "%ebx", "%ecx", "%edx"
    );

    return ( cycle_t)hi << 32 | lo; 
}
#else
# error "this compiler is not supported"
#endif

struct measure_cycles
{
    cycle_t operator()()
    {
        cycle_t start( cycles() );
        return cycles() - start;
    }
};

inline
cycle_t overhead_cycles()
{
    std::size_t iterations( 10);
    std::vector< cycle_t >  overhead( iterations, 0);
    for ( std::size_t i( 0); i < iterations; ++i)
        std::generate(
            overhead.begin(), overhead.end(),
            measure_cycles() );
    BOOST_ASSERT( overhead.begin() != overhead.end() );
    return std::accumulate( overhead.begin(), overhead.end(), 0) / iterations;
}

#endif // CYCLE_I386_H
