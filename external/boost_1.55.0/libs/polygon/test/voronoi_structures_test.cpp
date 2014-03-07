// Boost.Polygon library voronoi_structures_test.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <functional>
#include <vector>

#define BOOST_TEST_MODULE voronoi_structures_test
#include <boost/test/test_case_template.hpp>
#include <boost/polygon/detail/voronoi_structures.hpp>
using namespace boost::polygon::detail;

#include <boost/polygon/voronoi_geometry_type.hpp>
using namespace boost::polygon;

typedef point_2d<int> point_type;
typedef site_event<int> site_type;
typedef circle_event<int> circle_type;
typedef ordered_queue<int, std::greater<int> > ordered_queue_type;
typedef beach_line_node_key<int> node_key_type;
typedef beach_line_node_data<int, int> node_data_type;

BOOST_AUTO_TEST_CASE(point_2d_test1) {
  point_type p(1, 2);
  BOOST_CHECK_EQUAL(1, p.x());
  BOOST_CHECK_EQUAL(2, p.y());
  p.x(3);
  BOOST_CHECK_EQUAL(3, p.x());
  p.y(4);
  BOOST_CHECK_EQUAL(4, p.y());
}

BOOST_AUTO_TEST_CASE(site_event_test1) {
  site_type s(1, 2);
  s.sorted_index(1);
  s.initial_index(2);
  s.source_category(SOURCE_CATEGORY_SEGMENT_START_POINT);
  BOOST_CHECK_EQUAL(1, s.x0());
  BOOST_CHECK_EQUAL(1, s.x1());
  BOOST_CHECK_EQUAL(2, s.y0());
  BOOST_CHECK_EQUAL(2, s.y1());
  BOOST_CHECK(s.is_point());
  BOOST_CHECK(!s.is_segment());
  BOOST_CHECK(!s.is_inverse());
  BOOST_CHECK_EQUAL(1, s.sorted_index());
  BOOST_CHECK_EQUAL(2, s.initial_index());
  BOOST_CHECK_EQUAL(SOURCE_CATEGORY_SEGMENT_START_POINT, s.source_category());
}

BOOST_AUTO_TEST_CASE(site_event_test2) {
  site_type s(1, 2, 3, 4);
  s.sorted_index(1);
  s.initial_index(2);
  s.source_category(SOURCE_CATEGORY_INITIAL_SEGMENT);
  BOOST_CHECK_EQUAL(1, s.x0());
  BOOST_CHECK_EQUAL(2, s.y0());
  BOOST_CHECK_EQUAL(3, s.x1());
  BOOST_CHECK_EQUAL(4, s.y1());
  BOOST_CHECK(!s.is_point());
  BOOST_CHECK(s.is_segment());
  BOOST_CHECK(!s.is_inverse());
  BOOST_CHECK_EQUAL(SOURCE_CATEGORY_INITIAL_SEGMENT, s.source_category());

  s.inverse();
  BOOST_CHECK_EQUAL(3, s.x0());
  BOOST_CHECK_EQUAL(4, s.y0());
  BOOST_CHECK_EQUAL(1, s.x1());
  BOOST_CHECK_EQUAL(2, s.y1());
  BOOST_CHECK(s.is_inverse());
  BOOST_CHECK_EQUAL(SOURCE_CATEGORY_INITIAL_SEGMENT, s.source_category());
}

BOOST_AUTO_TEST_CASE(circle_event_test) {
  circle_type c(0, 1, 2);
  BOOST_CHECK_EQUAL(0, c.x());
  BOOST_CHECK_EQUAL(1, c.y());
  BOOST_CHECK_EQUAL(2, c.lower_x());
  BOOST_CHECK_EQUAL(1, c.lower_y());
  BOOST_CHECK(c.is_active());
  c.x(3);
  c.y(4);
  c.lower_x(5);
  BOOST_CHECK_EQUAL(3, c.x());
  BOOST_CHECK_EQUAL(4, c.y());
  BOOST_CHECK_EQUAL(5, c.lower_x());
  BOOST_CHECK_EQUAL(4, c.lower_y());
  c.deactivate();
  BOOST_CHECK(!c.is_active());
}

BOOST_AUTO_TEST_CASE(ordered_queue_test) {
  ordered_queue_type q;
  BOOST_CHECK(q.empty());
  std::vector<int*> vi;
  for (int i = 0; i < 20; ++i)
    vi.push_back(&q.push(i));
  for (int i = 0; i < 20; ++i)
    *vi[i] <<= 1;
  BOOST_CHECK(!q.empty());
  for (int i = 0; i < 20; ++i, q.pop())
    BOOST_CHECK_EQUAL(i << 1, q.top());
  BOOST_CHECK(q.empty());
}

BOOST_AUTO_TEST_CASE(beach_line_node_key_test) {
  node_key_type key(1);
  BOOST_CHECK_EQUAL(1, key.left_site());
  BOOST_CHECK_EQUAL(1, key.right_site());
  key.left_site(2);
  BOOST_CHECK_EQUAL(2, key.left_site());
  BOOST_CHECK_EQUAL(1, key.right_site());
  key.right_site(3);
  BOOST_CHECK_EQUAL(2, key.left_site());
  BOOST_CHECK_EQUAL(3, key.right_site());
}

BOOST_AUTO_TEST_CASE(beach_line_node_data_test) {
  node_data_type node_data(NULL);
  BOOST_CHECK(node_data.edge() == NULL);
  BOOST_CHECK(node_data.circle_event() == NULL);
  int data = 4;
  node_data.circle_event(&data);
  BOOST_CHECK(node_data.edge() == NULL);
  BOOST_CHECK(node_data.circle_event() == &data);
  node_data.edge(&data);
  BOOST_CHECK(node_data.edge() == &data);
  BOOST_CHECK(node_data.circle_event() == &data);
}
