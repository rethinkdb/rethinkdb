// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[comparable_distance
//` Shows how to efficiently get the closest point

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/numeric/conversion/bounds.hpp>
#include <boost/foreach.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    
    point_type p(1.4, 2.6);
    
    std::vector<point_type> v;
    for (double x = 0.0; x <= 4.0; x++)
    {
        for (double y = 0.0; y <= 4.0; y++)
        {
            v.push_back(point_type(x, y));
        }
    }
    
    point_type min_p;
    double min_d = boost::numeric::bounds<double>::highest();
    BOOST_FOREACH(point_type const& pv, v)
    {
        double d = boost::geometry::comparable_distance(p, pv);
        if (d < min_d)
        {
            min_d = d;
            min_p = pv;
        }
    }
    
    std::cout 
        << "Closest: " << boost::geometry::dsv(min_p) << std::endl
        << "At: " << boost::geometry::distance(p, min_p) << std::endl;

    return 0;
}

//]


//[comparable_distance_output
/*`
Output:
[pre
Closest: (1, 3)
At: 0.565685
]
*/
//]

