// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <rtree/interprocess/test_interprocess.hpp>

int test_main(int, char* [])
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> P2f;

    testset::interprocess::modifiers_and_additional<P2f>(bgi::dynamic_rstar(32, 8));
    
    return 0;
}
