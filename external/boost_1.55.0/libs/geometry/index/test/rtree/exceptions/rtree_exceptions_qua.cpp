// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <rtree/exceptions/test_exceptions.hpp>

int test_main(int, char* [])
{
    test_rtree_value_exceptions< bgi::quadratic<4, 2> >();
    test_rtree_value_exceptions(bgi::dynamic_quadratic(4, 2));

    test_rtree_elements_exceptions< bgi::quadratic_throwing<4, 2> >();

    return 0;
}
