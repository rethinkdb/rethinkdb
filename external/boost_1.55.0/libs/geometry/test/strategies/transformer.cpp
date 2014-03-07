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

#include <boost/geometry/strategies/transform/inverse_transformer.hpp>
#include <boost/geometry/strategies/transform/map_transformer.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>


#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/transform.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)


template <typename P, typename T>
void check_inverse(P const& p, T const& trans)
{
    typedef typename bg::coordinate_type<P>::type coordinate_type;
    const std::size_t dim = bg::dimension<P>::value;

    bg::strategy::transform::inverse_transformer<coordinate_type, dim, dim> inverse(trans);

    P i;
    bg::transform(p, i, inverse);

    BOOST_CHECK_CLOSE(double(bg::get<0>(i)), 1.0, 0.001);
    BOOST_CHECK_CLOSE(double(bg::get<1>(i)), 1.0, 0.001);
}

template <typename P>
void test_all()
{
    typedef typename bg::coordinate_type<P>::type coordinate_type;
    const std::size_t dim = bg::dimension<P>::value;

    P p;
    bg::assign_values(p, 1, 1);

    {
        bg::strategy::transform::translate_transformer<coordinate_type, dim, dim> trans(1, 1);
        P tp;
        bg::transform(p, tp, trans);

        BOOST_CHECK_CLOSE(double(bg::get<0>(tp)), 2.0, 0.001);
        BOOST_CHECK_CLOSE(double(bg::get<1>(tp)), 2.0, 0.001);

        check_inverse(tp, trans);
    }

    {
        bg::strategy::transform::scale_transformer<coordinate_type, dim, dim> trans(10, 10);
        P tp;
        bg::transform(p, tp, trans);

        BOOST_CHECK_CLOSE(double(bg::get<0>(tp)), 10.0, 0.001);
        BOOST_CHECK_CLOSE(double(bg::get<1>(tp)), 10.0, 0.001);

        check_inverse(tp, trans);
    }

    {
        bg::strategy::transform::rotate_transformer<bg::degree, double, dim, dim> trans(90.0);
        P tp;
        bg::transform(p, tp, trans);

        BOOST_CHECK_CLOSE(double(bg::get<0>(tp)), 1.0, 0.001);
        BOOST_CHECK_CLOSE(double(bg::get<1>(tp)), -1.0, 0.001);
        check_inverse(tp, trans);
    }

    {
        // Map from 0,0,2,2 to 0,0,500,500
        bg::strategy::transform::map_transformer<coordinate_type, dim, dim, false> trans
            (
                0.0, 0.0, 2.0, 2.0, 500, 500
            );
        P tp;
        bg::transform(p, tp, trans);

        BOOST_CHECK_CLOSE(double(bg::get<0>(tp)), 250.0, 0.001);
        BOOST_CHECK_CLOSE(double(bg::get<1>(tp)), 250.0, 0.001);

        check_inverse(tp, trans);
    }
}

int test_main(int, char* [])
{
    //test_all<int[2]>();
    //test_all<float[2]>();
    //test_all<double[2]>();

    test_all<boost::tuple<float, float> >();

    //test_all<point<int, 2, cs::cartesian> >();
    test_all<bg::model::point<float, 2, bg::cs::cartesian> >();
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
