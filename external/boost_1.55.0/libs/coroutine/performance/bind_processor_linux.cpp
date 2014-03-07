
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "bind_processor.hpp"

extern "C"
{
#include <pthread.h>
#include <sched.h>
}

#include <stdexcept>

#include <boost/config/abi_prefix.hpp>

void bind_to_processor( unsigned int n)
{
    cpu_set_t cpuset;
    CPU_ZERO( & cpuset);
    CPU_SET( n, & cpuset);

    int errno_( ::pthread_setaffinity_np( ::pthread_self(), sizeof( cpuset), & cpuset) );
    if ( errno_ != 0)
        throw std::runtime_error("::pthread_setaffinity_np() failed");
}

#include <boost/config/abi_suffix.hpp>
