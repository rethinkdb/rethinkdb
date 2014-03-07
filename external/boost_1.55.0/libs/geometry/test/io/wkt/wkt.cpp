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
#include <string>

#include <boost/algorithm/string.hpp>

#include <geometry_test_common.hpp>

#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/perimeter.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/topological_dimension.hpp>
#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>
#include <boost/variant/variant.hpp>

template <typename G>
void check_wkt(G const& geometry, std::string const& expected)
{
    std::ostringstream out;
    out << bg::wkt(geometry);
    BOOST_CHECK_EQUAL(boost::to_upper_copy(out.str()),
                      boost::to_upper_copy(expected));
}

template <typename G>
void test_wkt(std::string const& wkt, std::size_t n, double len = 0,
            double ar = 0, double peri = 0)
{
    G geometry;

    bg::read_wkt(wkt, geometry);

    /*
    std::cout << "n=" << bg::num_points(geometry)
        << " dim=" << bg::topological_dimension<G>::value
        << " length=" << bg::length(geometry)
        << " area=" << bg::area(geometry)
        << " perimeter=" << bg::perimeter(geometry)
        << std::endl << "\t\tgeometry=" << dsv(geometry)
        << std::endl;
    */

    BOOST_CHECK_EQUAL(bg::num_points(geometry), n);
    if (n > 0)
    {
        BOOST_CHECK_CLOSE(double(bg::length(geometry)), len, 0.0001);
        BOOST_CHECK_CLOSE(double(bg::area(geometry)), ar, 0.0001);
        BOOST_CHECK_CLOSE(double(bg::perimeter(geometry)), peri, 0.0001);
    }

    check_wkt(geometry, wkt);
    check_wkt(boost::variant<G>(geometry), wkt);
}

template <typename G>
void test_relaxed_wkt(std::string const& wkt, std::string const& expected)
{
    std::string e;
    G geometry;
    bg::read_wkt(wkt, geometry);
    std::ostringstream out;
    out << bg::wkt(geometry);

    BOOST_CHECK_EQUAL(boost::to_upper_copy(out.str()), boost::to_upper_copy(expected));
}




template <typename G>
void test_wrong_wkt(std::string const& wkt, std::string const& start)
{
    std::string e("no exception");
    G geometry;
    try
    {
        bg::read_wkt(wkt, geometry);
    }
    catch(bg::read_wkt_exception const& ex)
    {
        e = ex.what();
        boost::to_lower(e);
    }
    catch(...)
    {
        e = "other exception";
    }

    bool check = true;

#if defined(HAVE_TTMATH)
    // For ttmath we skip bad lexical casts
    typedef typename bg::coordinate_type<G>::type ct;

    if (boost::is_same<ct, ttmath_big>::type::value
        && boost::starts_with(start, "bad lexical cast"))
    {
        check = false;
    }
#endif

    if (check)
    {
        BOOST_CHECK_MESSAGE(boost::starts_with(e, start), "  Expected:"
                    << start << " Got:" << e << " with WKT: " << wkt);
    }
}

template <typename G>
void test_wkt_output_iterator(std::string const& wkt)
{
    G geometry;
    bg::read_wkt<G>(wkt, std::back_inserter(geometry));
}



#ifndef GEOMETRY_TEST_MULTI
template <typename T>
void test_all()
{
    using namespace boost::geometry;
    typedef bg::model::point<T, 2, bg::cs::cartesian> P;

    test_wkt<P>("POINT(1 2)", 1);
    test_wkt<bg::model::linestring<P> >("LINESTRING(1 1,2 2,3 3)", 3, 2 * sqrt(2.0));
    test_wkt<bg::model::polygon<P> >("POLYGON((0 0,0 4,4 4,4 0,0 0)"
            ",(1 1,1 2,2 2,2 1,1 1),(1 1,1 2,2 2,2 1,1 1))", 15, 0, 18, 24);

    // Non OGC: a box defined by a polygon
    //test_wkt<box<P> >("POLYGON((0 0,0 1,1 1,1 0,0 0))", 4, 0, 1, 4);
    test_wkt<bg::model::ring<P> >("POLYGON((0 0,0 1,1 1,1 0,0 0))", 5, 0, 1, 4);

    // We accept empty sequences as well (much better than EMPTY)...
    // ...or even POINT() (see below)
    test_wkt<bg::model::linestring<P> >("LINESTRING()", 0, 0);
    test_wkt<bg::model::polygon<P> >("POLYGON(())", 0);
    // ... or even with empty holes
    test_wkt<bg::model::polygon<P> >("POLYGON((),(),())", 0);
    // which all make no valid geometries, but they can exist.

    // These WKT's are incomplete or abnormal but they are considered OK
    test_relaxed_wkt<P>("POINT(1)", "POINT(1 0)");
    test_relaxed_wkt<P>("POINT()", "POINT(0 0)");
    test_relaxed_wkt<bg::model::linestring<P> >("LINESTRING(1,2,3)",
                "LINESTRING(1 0,2 0,3 0)");
    test_relaxed_wkt<P>("POINT  ( 1 2)   ", "POINT(1 2)");
    test_relaxed_wkt<P>("POINT  M ( 1 2)", "POINT(1 2)");
    test_relaxed_wkt<bg::model::box<P> >("BOX(1 1,2 2)", "POLYGON((1 1,1 2,2 2,2 1,1 1))");

    test_relaxed_wkt<bg::model::linestring<P> >("LINESTRING EMPTY", "LINESTRING()");

    test_relaxed_wkt<bg::model::polygon<P> >("POLYGON( ( ) , ( ) , ( ) )",
                "POLYGON((),(),())");

    // Wrong WKT's
    test_wrong_wkt<P>("POINT(1 2", "expected ')'");
    test_wrong_wkt<P>("POINT 1 2)", "expected '('");
    test_wrong_wkt<P>("POINT(1 2,)", "expected ')'");
    test_wrong_wkt<P>("POINT(1 2)foo", "too much tokens at 'foo'");
    test_wrong_wkt<P>("POINT(1 2 3)", "expected ')'");
    test_wrong_wkt<P>("POINT(a 2 3)", "bad lexical cast");
    test_wrong_wkt<P>("POINT 2 3", "expected '('");
    test_wrong_wkt<P>("POINT Z (1 2 3)", "z only allowed");

    test_wrong_wkt<P>("PIONT (1 2)", "should start with 'point'");

    test_wrong_wkt<bg::model::linestring<P> >("LINESTRING())", "too much tokens");

    test_wrong_wkt<bg::model::polygon<P> >("POLYGON((1 1,1 4,4 4,4 1,1 1)"
                ",((2 2,2 3,3 3,3 2,2 2))", "bad lexical cast");

    test_wrong_wkt<bg::model::box<P> >("BOX(1 1,2 2,3 3)", "box should have 2");
    test_wrong_wkt<bg::model::box<P> >("BOX(1 1,2 2) )", "too much tokens");

    if (boost::is_floating_point<T>::type::value
        || ! boost::is_fundamental<T>::type::value)
    {
        test_wkt<P>("POINT(1.1 2.1)", 1);
    }

    // Deprecated:
    // test_wkt_output_iterator<bg::model::linestring<P> >("LINESTRING(1 1,2 2,3 3)");
    // test_wkt_output_iterator<bg::model::ring<P> >("POLYGON((1 1,2 2,3 3))");
}
#endif


int test_main(int, char* [])
{
    test_all<double>();
    test_all<int>();

#if defined(HAVE_TTMATH)
    test_all<ttmath_big>();
#endif

    return 0;
}

/*

Results can be checked in PostGIS by query below,
or by MySQL (but replace length by glength and remove the perimeter)

Note:
- PostGIS gives "3" for a numpoints of a multi-linestring of 6 points in total (!)
    --> "npoints" should be taken for all geometries
- SQL Server 2008 gives "6"
    select geometry::STGeomFromText('MULTILINESTRING((1 1,2 2,3 3),(4 4,5 5,6 6))',0).STNumPoints()
- MySQL gives "NULL"

select 1 as code,'np p' as header,npoints(geomfromtext('POINT(1 2)')) as contents
union select 2,'length point', length(geomfromtext('POINT(1 2)'))
union select 3,'peri point', perimeter(geomfromtext('POINT(1 2)'))
union select 4,'area point',area(geomfromtext('POINT(1 2)'))


union select 5,'# ls',npoints(geomfromtext('LINESTRING(1 1,2 2,3 3)'))
union select 6,'length ls',length(geomfromtext('LINESTRING(1 1,2 2,3 3)'))
union select 7,'peri ls',perimeter(geomfromtext('LINESTRING(1 1,2 2,3 3)'))
union select 8,'aera ls',area(geomfromtext('LINESTRING(1 1,2 2,3 3)'))

union select 9,'# poly',npoints(geomfromtext('POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1),(1 1,1 2,2 2,2 1,1 1))'))
union select 10,'length poly',length(geomfromtext('POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1),(1 1,1 2,2 2,2 1,1 1))'))
union select 11,'peri poly',perimeter(geomfromtext('POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1),(1 1,1 2,2 2,2 1,1 1))'))
union select 12,'area poly',area(geomfromtext('POLYGON((0 0,0 4,4 4,4 0,0 0),(1 1,1 2,2 2,2 1,1 1),(1 1,1 2,2 2,2 1,1 1))'))

*/
