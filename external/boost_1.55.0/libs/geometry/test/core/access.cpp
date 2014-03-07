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


#include <boost/geometry/core/access.hpp>


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/algorithms/make.hpp>

#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/box.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename G>
void test_get_set()
{
    typedef typename bg::coordinate_type<G>::type coordinate_type;

    G g;
    bg::set<0>(g, coordinate_type(1));
    bg::set<1>(g, coordinate_type(2));

    coordinate_type x = bg::get<0>(g);
    coordinate_type y = bg::get<1>(g);

    BOOST_CHECK_CLOSE(double(x), 1.0, 0.0001);
    BOOST_CHECK_CLOSE(double(y), 2.0, 0.0001);
}

template <typename G>
void test_indexed_get_set(G& g)
{
    bg::set<0, 0>(g, 1);
    bg::set<0, 1>(g, 2);
    bg::set<1, 0>(g, 3);
    bg::set<1, 1>(g, 4);

    typedef typename bg::coordinate_type<G>::type coordinate_type;
    coordinate_type x1 = bg::get<0, 0>(g);
    coordinate_type y1 = bg::get<0, 1>(g);
    coordinate_type x2 = bg::get<1, 0>(g);
    coordinate_type y2 = bg::get<1, 1>(g);

    BOOST_CHECK_CLOSE(double(x1), 1.0, 0.0001);
    BOOST_CHECK_CLOSE(double(y1), 2.0, 0.0001);
    BOOST_CHECK_CLOSE(double(x2), 3.0, 0.0001);
    BOOST_CHECK_CLOSE(double(y2), 4.0, 0.0001);
}

template <typename G, typename T>
void test_indexed_get(G const& g, T a, T b, T c, T d)
{
    T x1 = bg::get<0, 0>(g);
    T y1 = bg::get<0, 1>(g);
    T x2 = bg::get<1, 0>(g);
    T y2 = bg::get<1, 1>(g);

    BOOST_CHECK_CLOSE(double(x1), double(a), 0.0001);
    BOOST_CHECK_CLOSE(double(y1), double(b), 0.0001);
    BOOST_CHECK_CLOSE(double(x2), double(c), 0.0001);
    BOOST_CHECK_CLOSE(double(y2), double(d), 0.0001);
}

template <typename P>
void test_all()
{
    typedef typename bg::coordinate_type<P>::type coordinate_type;

    // POINT, setting coordinate
    test_get_set<P>();


    // BOX, setting left/right/top/bottom
    bg::model::box<P> b;
    test_indexed_get_set(b);

    // SEGMENT (in GGL not having default constructor; however that is not a requirement)
    P p1 = bg::make_zero<P>();
    P p2 = bg::make_zero<P>();
    bg::model::referring_segment<P> s(p1, p2);
    test_indexed_get_set(s);

    // CONST SEGMENT
    bg::set<0>(p1, 1); // we don't use assign because dim in {2,3}
    bg::set<1>(p1, 2);
    bg::set<0>(p2, 3);
    bg::set<1>(p2, 4);
    bg::model::referring_segment<P const> cs(p1, p2);
    test_indexed_get(cs,
        coordinate_type(1), coordinate_type(2),
        coordinate_type(3), coordinate_type(4));
}


int test_main(int, char* [])
{
    test_get_set<int[2]>();
    test_get_set<float[2]>();
    test_get_set<double[2]>();
    test_get_set<double[3]>();

    test_get_set<boost::tuple<double, double> >();

    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
