// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Multipolygon DP simplification example from the mailing list discussion
// about the DP algorithm issue:
// http://lists.osgeo.org/pipermail/ggl/2011-September/001533.html

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/foreach.hpp>

int main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_xy;
    typedef boost::geometry::model::polygon<point_xy > polygon;
    typedef boost::geometry::model::ring<point_xy > ring;
    typedef boost::geometry::model::multi_polygon<polygon > multi_polygon;

    multi_polygon original_1;
    multi_polygon simplified_1;

    // Values between 0..1 and simplified with 1/2048
    boost::geometry::read_wkt("MULTIPOLYGON(((0.561648 1,1 1,1 0,0.468083 0,0.52758 0.00800554,0.599683 0.0280924,0.601611 0.265374,0.622693 0.316765,0.69507 0.357497,0.695623 0.429711,0.655111 0.502298,0.696467 0.543147,0.840712 0.593546,0.882583 0.66546,0.852357 0.748213,0.84264 0.789567,0.832667 0.841202,0.832667 0.841202,0.740538 0.873004,0.617349 0.905045,0.566576 0.977697,0.561648 1)),((0 0.801979,0.0308575 0.786234,0.0705513 0.631135,0.141616 0.527248,0.233985 0.505872,0.264777 0.526263,0.336631 0.505009,0.356603 0.422321,0.355803 0.350038,0.375252 0.205364,0.415206 0.0709182,0.45479 0,0 0,0 0,0 0.801979)))", original_1);

    std::cout << "Original: \n" << boost::geometry::num_points(original_1) << " points.\n\n";

    boost::geometry::simplify(original_1, simplified_1, 1.0 / 2048.0);

    std::cout << "Polygon with values 0..1 and simplified with 1.0 / 2048.0 \n"
        << "Result: \n" << boost::geometry::wkt(simplified_1) << "\n" << boost::geometry::num_points(simplified_1) << " points.\n\n";

    // Multiply every points from original_1 by 2047
    multi_polygon original_2(original_1);
    BOOST_FOREACH(polygon& p, original_2)
    {
        BOOST_FOREACH(point_xy& pt, p.outer())
        {
            pt.x(pt.x() * 2047.0);
            pt.y(pt.y() * 2047.0);
        }

        BOOST_FOREACH(ring& r, p.inners())
        {
            BOOST_FOREACH(point_xy& pt, r)
            {
                pt.x(pt.x() * 2047.0);
                pt.y(pt.y() * 2047.0);
            }
        }
    }

    multi_polygon simplified_2;
    boost::geometry::simplify(original_2, simplified_2, 1.0);
    std::cout << "Same values but multiplied by 2047.0 and simplified with 1.0\n"
        << "Result: \n" << boost::geometry::wkt(simplified_2) << "\n" << boost::geometry::num_points(simplified_2) << " points.\n";

    return 0;
}

