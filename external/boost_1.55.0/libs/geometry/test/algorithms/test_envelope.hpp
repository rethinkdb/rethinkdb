// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_ENVELOPE_HPP
#define BOOST_GEOMETRY_TEST_ENVELOPE_HPP


#include <geometry_test_common.hpp>

#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/io/wkt/read.hpp>


template<typename Box, std::size_t DimensionCount>
struct check_result
{};

template <typename Box>
struct check_result<Box, 2>
{
    typedef typename bg::coordinate_type<Box>::type ctype;
    typedef typename boost::mpl::if_
            <
                boost::is_arithmetic<ctype>,
                double,
                ctype
            >::type type;

    static void apply(Box const& b, const type& x1, const type& y1, const type& z1,
                const type& x2, const type& y2, const type& z2)
    {
        BOOST_CHECK_CLOSE((bg::get<bg::min_corner, 0>(b)), x1, 0.001);
        BOOST_CHECK_CLOSE((bg::get<bg::min_corner, 1>(b)), y1, 0.001);

        BOOST_CHECK_CLOSE((bg::get<bg::max_corner, 0>(b)), x2, 0.001);
        BOOST_CHECK_CLOSE((bg::get<bg::max_corner, 1>(b)), y2, 0.001);
    }
};

template <typename Box>
struct check_result<Box, 3>
{
    typedef typename bg::coordinate_type<Box>::type ctype;
    typedef typename boost::mpl::if_
            <
                boost::is_arithmetic<ctype>,
                double,
                ctype
            >::type type;

    static void apply(Box const& b, const type& x1, const type& y1, const type& z1,
                const type& x2, const type& y2, const type& z2)
    {
        BOOST_CHECK_CLOSE((bg::get<bg::min_corner, 0>(b)), x1, 0.001);
        BOOST_CHECK_CLOSE((bg::get<bg::min_corner, 1>(b)), y1, 0.001);
        BOOST_CHECK_CLOSE((bg::get<bg::min_corner, 2>(b)), z1, 0.001);

        BOOST_CHECK_CLOSE((bg::get<bg::max_corner, 0>(b)), x2, 0.001);
        BOOST_CHECK_CLOSE((bg::get<bg::max_corner, 1>(b)), y2, 0.001);
        BOOST_CHECK_CLOSE((bg::get<bg::max_corner, 2>(b)), z2, 0.001);
    }
};


template <typename Geometry, typename T>
void test_envelope(std::string const& wkt,
                   const T& x1, const T& x2,
                   const T& y1, const T& y2,
                   const T& z1 = 0, const T& z2 = 0)
{
    typedef bg::model::box<typename bg::point_type<Geometry>::type > box_type;

    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    box_type b;
    bg::envelope(geometry, b);

    check_result<box_type, bg::dimension<Geometry>::type::value>::apply(b, x1, y1, z1, x2, y2, z2);
}

template <typename Geometry, typename T>
void test_envelope_strategy(std::string const& wkt,
                   const T& x1, const T& x2,
                   const T& y1, const T& y2,
                   const T& z1 = 0, const T& z2 = 0)
{
    typedef bg::model::box<typename bg::point_type<Geometry>::type > box_type;

    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    box_type b;
    bg::envelope(geometry, b);

    check_result<box_type, bg::dimension<Geometry>::type::value>::apply(b, x1, y1, z1, x2, y2, z2);
}



#endif
