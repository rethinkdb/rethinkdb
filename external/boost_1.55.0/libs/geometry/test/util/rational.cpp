// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/util/rational.hpp>

void test_coordinate_cast(std::string const& s, int expected_nom, int expected_denom)
{
	boost::rational<int> a = bg::detail::coordinate_cast<boost::rational<int> >::apply(s);
    BOOST_CHECK_EQUAL(a.numerator(), expected_nom);
    BOOST_CHECK_EQUAL(a.denominator(), expected_denom);
}


void test_wkt(std::string const& wkt, std::string const expected_wkt)
{
    bg::model::point<boost::rational<int>, 2, bg::cs::cartesian> p;
    bg::read_wkt(wkt, p);
    std::ostringstream out;
    out << bg::wkt(p);

    BOOST_CHECK_EQUAL(out.str(), expected_wkt);
}

int test_main(int, char* [])
{
    test_coordinate_cast("0", 0, 1);
    test_coordinate_cast("1", 1, 1);
    test_coordinate_cast("-1", -1, 1);
    test_coordinate_cast("-0.5", -1, 2);
    test_coordinate_cast("-1.5", -3, 2);
    test_coordinate_cast("0.5", 1, 2);
    test_coordinate_cast("1.5", 3, 2);
    test_coordinate_cast("2.12345", 42469, 20000);
    test_coordinate_cast("1.", 1, 1);

    test_coordinate_cast("3/2", 3, 2);
    test_coordinate_cast("-3/2", -3, 2);

    test_wkt("POINT(1.5 2.75)", "POINT(3/2 11/4)");
    test_wkt("POINT(3/2 11/4)", "POINT(3/2 11/4)");
    test_wkt("POINT(-1.5 2.75)", "POINT(-3/2 11/4)");
    test_wkt("POINT(-3/2 11/4)", "POINT(-3/2 11/4)");

    return 0;
}
