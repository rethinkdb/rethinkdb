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


#include <boost/geometry/views/detail/range_type.hpp>
#include <boost/geometry/util/as_range.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/read.hpp>

template <int D, typename Range>
double sum(Range const& range)
{
    double s = 0.0;
    for (typename boost::range_const_iterator<Range>::type it = boost::begin(range);
        it != boost::end(range); ++it)
    {
        s += bg::get<D>(*it);
    }
    return s;
}

template <typename G>
void test_geometry(std::string const& wkt, double expected_x, double expected_y)
{
    G geometry;

    // Declare a range-type, compatible with boost::range,
    // such that range_iterator etc could be called
    typedef typename bg::detail::range_type<G>::type range_type;

    bg::read_wkt(wkt, geometry);

    double s = sum<0>(bg::as_range<range_type>(geometry));
    BOOST_CHECK_CLOSE(s, expected_x, 0.001);

    s = sum<1>(bg::as_range<range_type>(geometry));
    BOOST_CHECK_CLOSE(s, expected_y, 0.001);
}


template <typename P>
void test_all()
{
    // As-range utility should consider a geometry as a range, so
    // linestring stays linestring
    test_geometry<bg::model::linestring<P> >("LINESTRING(1 2,3 4)", 4, 6);

    // polygon will only be outer-ring
    test_geometry<bg::model::polygon<P> >("POLYGON((1 2,3 4))", 4, 6);
    test_geometry<bg::model::polygon<P> >("POLYGON((1 2,3 4),(5 6,7 8,9 10))", 4, 6);

    // the utility is useful for:
    // - convex hull (holes do not count)
    // - envelope (idem)
}

int test_main(int, char* [])
{
    test_all<bg::model::point<double, 2, bg::cs::cartesian> >();

    return 0;
}
