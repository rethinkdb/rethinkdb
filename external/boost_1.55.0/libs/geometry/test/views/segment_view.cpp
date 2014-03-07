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

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/views/segment_view.hpp>
#include <boost/geometry/io/wkt/read.hpp>



template <typename Segment>
void test_geometry(std::string const& wkt, std::string const& expected)
{

    Segment segment;
    bg::read_wkt(wkt, segment);

    typedef bg::segment_view<Segment> range_type;
    range_type range(segment);

    {
        std::ostringstream out;
        for (typename boost::range_iterator<range_type>::type it = boost::begin(range);
            it != boost::end(range); ++it)
        {
            out << " " << bg::get<0>(*it) << bg::get<1>(*it);
        }
        BOOST_CHECK_EQUAL(out.str(), expected);
    }

    {
        // Check forward/backward behaviour
        std::ostringstream out;
        typename boost::range_iterator<range_type>::type it = boost::begin(range);
        it++;
        it--;
        out << " " << bg::get<0>(*it) << bg::get<1>(*it);
        typename boost::range_iterator<range_type>::type it2 = boost::end(range);
        it2--;
        out << " " << bg::get<0>(*it2) << bg::get<1>(*it2);
        BOOST_CHECK_EQUAL(out.str(), expected);
    }

    {
        // Check random access behaviour
        int const n = boost::size(range);
        BOOST_CHECK_EQUAL(n, 2);
    }

    // Check Boost.Range concept
    BOOST_CONCEPT_ASSERT( (boost::RandomAccessRangeConcept<range_type>) );

}


template <typename P>
void test_all()
{
    test_geometry<bg::model::segment<P> >("linestring(1 1,2 2)", " 11 22");
    test_geometry<bg::model::segment<P> >("linestring(4 4,3 3)", " 44 33");
}


int test_main(int, char* [])
{
    std::vector<int> a; 
    a.push_back(1);
    boost::range_iterator<std::vector<int> const>::type it = a.end();
    --it;
    std::cout << *it << std::endl;
    test_all<bg::model::d2::point_xy<double> >();
    return 0;
}
