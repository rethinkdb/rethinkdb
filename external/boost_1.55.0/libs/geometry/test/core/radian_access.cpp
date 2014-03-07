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

#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/geometries/point.hpp>


template <std::size_t D, typename P, typename T>
void test_get(T const& c, T const& e)
{
    //std::cout << "get_as_radian " << typeid(P).name() << std::endl;

    typedef typename bg::coordinate_type<P>::type coordinate_type;

    P p;
    bg::set<D>(p, coordinate_type(c));

    coordinate_type g = bg::get_as_radian<D>(p);

    BOOST_CHECK_CLOSE(double(g), double(e), 0.0001);
}


template <std::size_t D, typename P, typename T>
void test_set(T const& c, T const& e)
{
    //std::cout << "set_from_radian " << typeid(P).name() << std::endl;

    typedef typename bg::coordinate_type<P>::type coordinate_type;

    P p;
    bg::set_from_radian<D>(p, coordinate_type(c));

    coordinate_type g = bg::get<D>(p);

    BOOST_CHECK_CLOSE(double(g), double(e), 0.0001);
}


template <typename T>
void test()
{
    double d2r = 3.1415926535897932384626433832795 / 180.0;

    // Degree
    test_get<0, bg::model::point<T, 2, bg::cs::spherical<bg::degree> > >
        (1.0, 1.0 * d2r);
    test_get<1, bg::model::point<T, 2, bg::cs::spherical<bg::degree> > >
        (2.0, 2.0 * d2r);

    test_set<0, bg::model::point<T, 2, bg::cs::spherical<bg::degree> > >
        (1.0, 1.0 / d2r);
    test_set<1, bg::model::point<T, 2, bg::cs::spherical<bg::degree> > >
        (2.0, 2.0 / d2r);


    // Radian
    test_get<0, bg::model::point<T, 2, bg::cs::spherical<bg::radian> > >
        (1.0, 1.0);
    test_get<1, bg::model::point<T, 2, bg::cs::spherical<bg::radian> > >
        (1.0, 1.0);

    test_set<0, bg::model::point<T, 2, bg::cs::spherical<bg::radian> > >
        (1.0, 1.0);
    test_set<1, bg::model::point<T, 2, bg::cs::spherical<bg::radian> > >
        (1.0, 1.0);


    // Cartesian (== untouched, no conversion)
    test_get<0, bg::model::point<T, 2, bg::cs::cartesian> >
        (1.0, 1.0);
    test_get<1, bg::model::point<T, 2, bg::cs::cartesian> >
        (1.0, 1.0);

    // Dimension >=2, should always be untouched, even for degree
    // (assuming lat/lon + height)
    test_set<2, bg::model::point<T, 3, bg::cs::spherical<bg::radian> > >
        (1.0, 1.0);
    test_set<2, bg::model::point<T, 3, bg::cs::spherical<bg::degree> > >
        (1.0, 1.0);

}




int test_main(int, char* [])
{
    //test<float>();
    test<double>();

    return 0;
}
