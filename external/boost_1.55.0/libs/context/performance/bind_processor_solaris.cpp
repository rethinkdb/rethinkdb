
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "bind_processor.hpp"

extern "C"
{
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
}

#include <stdexcept>

#include <boost/config/abi_prefix.hpp>

void bind_to_processor( unsigned int n)
{
    if ( ::processor_bind( P_LWPID, P_MYID, static_cast< processorid_t >( n), 0) == -1)
        throw std::runtime_error("::processor_bind() failed");
}

#include <boost/config/abi_suffix.hpp>
