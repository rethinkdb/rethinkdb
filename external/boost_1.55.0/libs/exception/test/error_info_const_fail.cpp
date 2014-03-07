//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/get_error_info.hpp>

typedef boost::error_info<struct foo_,int> foo;

void
test( boost::exception const * e )
    {
    ++*boost::get_error_info<foo>(*e);
    }
