//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/to_string.hpp>

namespace
n1
    {
    struct
    c1
        {
        };
    }

int
tester()
    {
    using namespace boost;
    (void) to_string(n1::c1());
    return 1;
    }
