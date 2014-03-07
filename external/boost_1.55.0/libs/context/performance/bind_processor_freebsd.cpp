
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "bind_processor.hpp"

extern "C"
{
#include <sys/param.h>
#include <sys/cpuset.h>
}

#include <stdexcept>

#include <boost/config/abi_prefix.hpp>

void bind_to_processor( unsigned int n)
{
    cpuset_t cpuset;
    CPU_ZERO( & cpuset);
    CPU_SET( n, & cpuset);

    if ( ::cpuset_setaffinity(  CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof( cpuset), & cpuset) == -1)
        throw std::runtime_error("::cpuset_setaffinity() failed");
}

#include <boost/config/abi_suffix.hpp>
