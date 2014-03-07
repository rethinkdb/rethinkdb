// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

#include <geometry_test_common.hpp>

#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/dsv/write.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian);


template <typename View>
void test_option(View const& view, std::string const& expected)
{

    bool first = true;
    std::ostringstream out;

    typedef typename boost::range_iterator<View const>::type iterator;

    iterator end = boost::end(view);
    for (iterator it = boost::begin(view); it != end; ++it, first = false)
    {
        out << (first ? "" : " ") << bg::dsv(*it);
    }
    BOOST_CHECK_EQUAL(out.str(), expected);
}

template <bg::closure_selector Closure, bg::iterate_direction Direction, typename Range>
void test_close_reverse(Range const& range, std::string const& expected)
{
    typedef typename bg::reversible_view<Range const, Direction>::type rview;
    typedef typename bg::closeable_view<rview const, Closure>::type cview;

    rview view1(range);
    cview view2(view1);

    test_option(view2, expected);
}


template <bg::iterate_direction Direction, bg::closure_selector Closure, typename Range>
void test_reverse_close(Range const& range, std::string const& expected)
{
    typedef typename bg::closeable_view<Range const, Closure>::type cview;
    typedef typename bg::reversible_view<cview const, Direction>::type rview;

    cview view1(range);
    rview view2(view1);
    test_option(view2, expected);
}


template
<
    bg::iterate_direction Direction1,
    bg::iterate_direction Direction2,
    typename Range
>
void test_reverse_reverse(Range const& range, std::string const& expected)
{
    typedef typename bg::reversible_view<Range const, Direction1>::type rview1;
    typedef typename bg::reversible_view<rview1 const, Direction2>::type rview2;

    rview1 view1(range);
    rview2 view2(view1);
    test_option(view2, expected);
}

template
<
    bg::closure_selector Closure1,
    bg::closure_selector Closure2,
    typename Range
>
void test_close_close(Range const& range, std::string const& expected)
{
    typedef typename bg::closeable_view<Range const, Closure1>::type cview1;
    typedef typename bg::closeable_view<cview1 const, Closure2>::type cview2;

    cview1 view1(range);
    cview2 view2(view1);
    test_option(view2, expected);
}


template <typename Geometry>
void test_geometry(std::string const& wkt,
            std::string const& expected_n,
            std::string const& expected_r,
            std::string const& closing,
            std::string const& rclosing
            )
{
    std::string expected;
    Geometry geo;
    bg::read_wkt(wkt, geo);

    test_close_reverse<bg::closed, bg::iterate_forward>(geo, expected_n);
    test_close_reverse<bg::open, bg::iterate_forward>(geo, expected_n + closing);
    test_close_reverse<bg::closed, bg::iterate_reverse>(geo, expected_r);

    // 13-12-2010, this case was problematic in MSVC
    // SOLVED: caused by IMPLICIT constructor! It is now explicit and should be kept like that.
    test_close_reverse<bg::open, bg::iterate_reverse>(geo, expected_r + rclosing);

    test_reverse_close<bg::iterate_forward, bg::closed>(geo, expected_n);
    test_reverse_close<bg::iterate_forward, bg::open>(geo, expected_n + closing);
    test_reverse_close<bg::iterate_reverse, bg::closed>(geo, expected_r);

    // first closed, then reversed:
    expected = boost::trim_copy(closing + " " + expected_r);
    test_reverse_close<bg::iterate_reverse, bg::open>(geo, expected);

    test_reverse_reverse<bg::iterate_forward, bg::iterate_forward>(geo, expected_n);
    test_reverse_reverse<bg::iterate_reverse, bg::iterate_reverse>(geo, expected_n);
    test_reverse_reverse<bg::iterate_forward, bg::iterate_reverse>(geo, expected_r);
    test_reverse_reverse<bg::iterate_reverse, bg::iterate_forward>(geo, expected_r);

    test_close_close<bg::closed, bg::closed>(geo, expected_n);
    test_close_close<bg::open, bg::closed>(geo, expected_n + closing);
    test_close_close<bg::closed, bg::open>(geo, expected_n + closing);
    test_close_close<bg::open, bg::open>(geo, expected_n + closing + closing);
}


template <typename P>
void test_all()
{
    test_geometry<bg::model::ring<P> >(
            "POLYGON((1 1,1 4,4 4,4 1))",
            "(1, 1) (1, 4) (4, 4) (4, 1)",
            "(4, 1) (4, 4) (1, 4) (1, 1)",
            " (1, 1)", // closing
            " (4, 1)" // rclosing
            );
}


int test_main(int, char* [])
{
    test_all<bg::model::d2::point_xy<double> >();
    test_all<bg::model::point<int, 2, bg::cs::cartesian> >();
    test_all<boost::tuple<double, double> >();

    return 0;
}
