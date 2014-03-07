//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/exception.hpp>
#include <stdexcept>
#include <string>

namespace
boost
    {
    namespace
    exception_test
        {
        void
        throw_length_error()
            {
            throw enable_error_info( std::length_error("exception test length error") );
            }
        }
    }
