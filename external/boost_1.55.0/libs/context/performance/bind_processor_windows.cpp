
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "bind_processor.hpp"

extern "C"
{
#include <windows.h>
}

#include <stdexcept>

#include <boost/config/abi_prefix.hpp>

void bind_to_processor( unsigned int n)
{
    if ( ::SetThreadAffinityMask( ::GetCurrentThread(), ( DWORD_PTR)1 << n) == 0)
        throw std::runtime_error("::SetThreadAffinityMask() failed");
}

#include <boost/config/abi_suffix.hpp>
