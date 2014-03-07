// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[dimension
//` Examine the number of coordinates making up the points in a linestring type

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian);

int main()
{
    int dim = boost::geometry::dimension
        <
            boost::geometry::model::linestring
                <
                    boost::tuple<float, float, float>
                >
        >::value;
    
    std::cout << "dimensions: " << dim << std::endl;

    return 0;
}

//]


//[dimension_output
/*`
Output:
[pre
dimensions: 3
]
*/
//]
