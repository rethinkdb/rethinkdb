// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[for_each_point
//` Convenient usage of for_each_point, rounding all points of a geometry

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>



template <typename Point>
class round_coordinates
{
private :
    typedef typename boost::geometry::coordinate_type<Point>::type coordinate_type;
    coordinate_type factor;

    inline coordinate_type round(coordinate_type value)
    {
        return floor(0.5 + (value / factor)) * factor;
    }

public :
    round_coordinates(coordinate_type f)
        : factor(f)
    {}

    inline void operator()(Point& p)
    {
        using boost::geometry::get;
        using boost::geometry::set;
        set<0>(p, round(get<0>(p)));
        set<1>(p, round(get<1>(p)));
    }
};


int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point;
    boost::geometry::model::polygon<point> poly;
    boost::geometry::read_wkt("POLYGON((0 0,1.123 9.987,8.876 2.234,0 0),(3.345 4.456,7.654 8.765,9.123 5.432,3.345 4.456))", poly);
    boost::geometry::for_each_point(poly, round_coordinates<point>(0.1));
    std::cout << "Rounded: " << boost::geometry::wkt(poly) << std::endl;
    return 0;
}

//]


//[for_each_point_output
/*`
Output:
[pre
 Rounded: POLYGON((0 0,1.1 10,8.9 2.2,0 0),(3.3 4.5,7.7 8.8,9.1 5.4,3.3 4.5))
]
*/
//]
