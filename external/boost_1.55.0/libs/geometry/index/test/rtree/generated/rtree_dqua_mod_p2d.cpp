// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <rtree/test_rtree.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

int test_main(int, char* [])
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> Indexable;
    testset::modifiers<Indexable>(bgi::dynamic_quadratic(5, 2), std::allocator<int>());
    return 0;
}
