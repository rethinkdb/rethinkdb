// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_RELATE_HPP
#define BOOST_GEOMETRY_TEST_RELATE_HPP

#include <string>


static std::string disjoint_simplex[2] =
    {"POLYGON((0 0,0 2,2 2,0 0))",
    "POLYGON((1 0,3 2,3 0,1 0))"};

static std::string touch_simplex[2] =
    {"POLYGON((0 0,0 2,2 2,0 0))",
    "POLYGON((2 2,3 2,3 0,2 2))"};

static std::string overlaps_box[2] =
    {"POLYGON((0 0,0 2,2 2,0 0))",
    "POLYGON((1 1,3 2,3 0,1 1))"};

static std::string within_simplex[2] =
    {"POLYGON((0 0,1 4,4 1,0 0))",
    "POLYGON((1 1,1 3,3 1,1 1))"};



#endif
