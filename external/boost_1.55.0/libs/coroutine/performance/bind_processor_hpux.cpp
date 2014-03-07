
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "bind_processor.hpp"

extern "C"
{
#include <sys/pthread.h>
}

#include <stdexcept>

#include <boost/config/abi_prefix.hpp>

void bind_to_processor( unsigned int n)
{
    ::pthread_spu_t spu;
    int errno_(
        ::pthread_processor_bind_np(
            PTHREAD_BIND_FORCED_NP,
            & spu,
            static_cast< pthread_spu_t >( n),
            PTHREAD_SELFTID_NP) );
    if ( errno_ != 0)
        throw std::runtime_error("::pthread_processor_bind_np() failed");
}

#include <boost/config/abi_suffix.hpp>
