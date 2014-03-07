// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithms/test_overlaps.hpp>


#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>


template <typename P>
void test_2d()
{
#if defined(BOOST_GEOMETRY_COMPILE_FAIL)
    test_geometry<P, P>("POINT(1 1)", "POINT(1 1)", true);
#endif

    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 3 3)", "BOX(0 0,2 2)", true);

    // touch -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 3 3)", "BOX(3 3,5 5)", false);

    // disjoint -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 3 3)", "BOX(4 4,6 6)", false);

    // within -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 5 5)", "BOX(2 2,3 3)", false);

    // within+touch -> false
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1, 5 5)", "BOX(2 2,5 5)", false);
}

template <typename P>
void test_3d()
{
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1 1, 3 3 3)", "BOX(0 0 0,2 2 2)", true);
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1 1, 3 3 3)", "BOX(3 3 3,5 5 5)", false);
    test_geometry<bg::model::box<P>, bg::model::box<P> >("BOX(1 1 1, 3 3 3)", "BOX(4 4 4,6 6 6)", false);
}



int test_main( int , char* [] )
{
    test_2d<bg::model::d2::point_xy<int> >();
    test_2d<bg::model::d2::point_xy<double> >();

#if defined(HAVE_TTMATH)
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
#endif

   //test_3d<bg::model::point<double, 3, bg::cs::cartesian> >();

    return 0;
}
