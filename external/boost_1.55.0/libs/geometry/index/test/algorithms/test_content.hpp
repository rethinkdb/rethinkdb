// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_TEST_CONTENT_HPP
#define BOOST_GEOMETRY_INDEX_TEST_CONTENT_HPP

#include <geometry_index_test_common.hpp>

#include <boost/geometry/index/detail/algorithms/content.hpp>

//#include <boost/geometry/io/wkt/read.hpp>


template <typename Geometry>
void test_content(Geometry const& geometry,
            typename bgi::detail::default_content_result<Geometry>::type expected_value)
{
    typename bgi::detail::default_content_result<Geometry>::type value = bgi::detail::content(geometry);

#ifdef GEOMETRY_TEST_DEBUG
    std::ostringstream out;
    out << typeid(typename bg::coordinate_type<Geometry>::type).name()
        << " "
        << typeid(typename bgi::detail::default_content_result<Geometry>::type).name()
        << " "
        << "content : " << value
        << std::endl;
    std::cout << out.str();
#endif

    BOOST_CHECK_CLOSE(value, expected_value, 0.0001);
}

template <typename Geometry>
void test_geometry(std::string const& wkt,
            typename bgi::detail::default_content_result<Geometry>::type expected_value)
{
    Geometry geometry;
    bg::read_wkt(wkt, geometry);
    test_content(geometry, expected_value);
}

#endif
