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


#include <sstream>

#include <geometry_test_common.hpp>

#include <boost/geometry/io/dsv/write.hpp>


#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/read.hpp>



template <typename G>
void test_dsv(std::string const& wkt, std::string const& dsv)
{
    G geometry;

    bg::read_wkt(wkt, geometry);

    std::ostringstream out;
    out << bg::dsv(geometry);
    BOOST_CHECK_EQUAL(out.str(), dsv);
}


#ifndef GEOMETRY_TEST_MULTI
template <typename T>
void test_all()
{
    using namespace boost::geometry;
    typedef model::point<T, 2, cs::cartesian> P;

    test_dsv<P >("POINT(1 2)", "(1, 2)");
    test_dsv<model::linestring<P> >(
        "LINESTRING(1 1,2 2,3 3)",
        "((1, 1), (2, 2), (3, 3))");
    test_dsv<model::polygon<P> >("POLYGON((0 0,0 4,4 4,4 0,0 0))",
        "(((0, 0), (0, 4), (4, 4), (4, 0), (0, 0)))");

    test_dsv<model::ring<P> >("POLYGON((0 0,0 4,4 4,4 0,0 0))",
        "((0, 0), (0, 4), (4, 4), (4, 0), (0, 0))");

    test_dsv<model::box<P> >("BOX(0 0,1 1)",
        "((0, 0), (1, 1))");
}
#endif


int test_main(int, char* [])
{
    test_all<double>();
    test_all<int>();


    return 0;
}

