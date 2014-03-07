// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[assign_2d_point
//` Shows the usage of assign to set point coordinates, and, besides that, shows how you can initialize ttmath points with high precision

#include <iostream>
#include <iomanip>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#if defined(HAVE_TTMATH)
#  include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif


int main()
{
    using boost::geometry::assign_values;


    boost::geometry::model::d2::point_xy<double> p1;
    assign_values(p1, 1.2345, 2.3456);

#if defined(HAVE_TTMATH)
    boost::geometry::model::d2::point_xy<ttmath::Big<1,4> > p2;
    assign_values(p2, "1.2345", "2.3456"); /*< It is possible to assign coordinates with other types than the coordinate type.
        For ttmath, you can e.g. conveniently use strings. The advantage is that it then has higher precision, because
        if doubles are used for assignments the double-precision is used.
        >*/
#endif

    std::cout
        << std::setprecision(20)
        << boost::geometry::dsv(p1) << std::endl
#if defined(HAVE_TTMATH)
        << boost::geometry::dsv(p2) << std::endl
#endif
        ;

    return 0;
}

//]


//[assign_2d_point_output
/*`
Output:
[pre
(1.2344999999999999, 2.3456000000000001)
(1.2345, 2.3456)
]
*/
//]
