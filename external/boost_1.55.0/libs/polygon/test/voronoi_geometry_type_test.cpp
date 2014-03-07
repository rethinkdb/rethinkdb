// Boost.Polygon library voronoi_geometry_type_test.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#define BOOST_TEST_MODULE voronoi_geometry_type_test

#include <boost/test/test_case_template.hpp>
#include <boost/polygon/voronoi_geometry_type.hpp>
using namespace boost::polygon;

BOOST_AUTO_TEST_CASE(source_category_test1) {
  BOOST_CHECK(belongs(SOURCE_CATEGORY_SINGLE_POINT, GEOMETRY_CATEGORY_POINT));
  BOOST_CHECK(belongs(SOURCE_CATEGORY_SEGMENT_START_POINT, GEOMETRY_CATEGORY_POINT));
  BOOST_CHECK(belongs(SOURCE_CATEGORY_SEGMENT_END_POINT, GEOMETRY_CATEGORY_POINT));
  BOOST_CHECK(!belongs(SOURCE_CATEGORY_INITIAL_SEGMENT, GEOMETRY_CATEGORY_POINT));
  BOOST_CHECK(!belongs(SOURCE_CATEGORY_REVERSE_SEGMENT, GEOMETRY_CATEGORY_POINT));

  BOOST_CHECK(!belongs(SOURCE_CATEGORY_SINGLE_POINT, GEOMETRY_CATEGORY_SEGMENT));
  BOOST_CHECK(!belongs(SOURCE_CATEGORY_SEGMENT_START_POINT, GEOMETRY_CATEGORY_SEGMENT));
  BOOST_CHECK(!belongs(SOURCE_CATEGORY_SEGMENT_END_POINT, GEOMETRY_CATEGORY_SEGMENT));
  BOOST_CHECK(belongs(SOURCE_CATEGORY_INITIAL_SEGMENT, GEOMETRY_CATEGORY_SEGMENT));
  BOOST_CHECK(belongs(SOURCE_CATEGORY_REVERSE_SEGMENT, GEOMETRY_CATEGORY_SEGMENT));
}
