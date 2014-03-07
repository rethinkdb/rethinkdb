// Boost.Polygon library voronoi_predicates_test.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <limits>
#include <map>

#define BOOST_TEST_MODULE voronoi_predicates_test
#include <boost/test/test_case_template.hpp>

#include <boost/polygon/detail/voronoi_ctypes.hpp>
#include <boost/polygon/detail/voronoi_predicates.hpp>
#include <boost/polygon/detail/voronoi_structures.hpp>
using namespace boost::polygon::detail;

#include <boost/polygon/voronoi_geometry_type.hpp>
using namespace boost::polygon;

ulp_comparison<double> ulp_cmp;

typedef voronoi_predicates< voronoi_ctype_traits<int> > VP;
typedef point_2d<int> point_type;
typedef site_event<int> site_type;
typedef circle_event<double> circle_type;
VP::event_comparison_predicate<site_type, circle_type> event_comparison;

typedef beach_line_node_key<site_type> key_type;
typedef VP::distance_predicate<site_type> distance_predicate_type;
typedef VP::node_comparison_predicate<key_type> node_comparison_type;
typedef std::map<key_type, int, node_comparison_type> beach_line_type;
typedef beach_line_type::iterator bieach_line_iterator;
distance_predicate_type distance_predicate;
node_comparison_type node_comparison;

typedef VP::circle_existence_predicate<site_type> CEP_type;
typedef VP::mp_circle_formation_functor<site_type, circle_type> MP_CFF_type;
typedef VP::lazy_circle_formation_functor<site_type, circle_type> lazy_CFF_type;
VP::circle_formation_predicate<site_type, circle_type, CEP_type, MP_CFF_type> mp_predicate;
VP::circle_formation_predicate<site_type, circle_type, CEP_type, lazy_CFF_type> lazy_predicate;

#define CHECK_ORIENTATION(P1, P2, P3, R1, R2) \
    BOOST_CHECK_EQUAL(VP::ot::eval(P1, P2, P3) == R1, true); \
    BOOST_CHECK_EQUAL(VP::ot::eval(P1, P3, P2) == R2, true); \
    BOOST_CHECK_EQUAL(VP::ot::eval(P2, P1, P3) == R2, true); \
    BOOST_CHECK_EQUAL(VP::ot::eval(P2, P3, P1) == R1, true); \
    BOOST_CHECK_EQUAL(VP::ot::eval(P3, P1, P2) == R1, true); \
    BOOST_CHECK_EQUAL(VP::ot::eval(P3, P2, P1) == R2, true)

#define CHECK_EVENT_COMPARISON(A, B, R1, R2) \
    BOOST_CHECK_EQUAL(event_comparison(A, B), R1); \
    BOOST_CHECK_EQUAL(event_comparison(B, A), R2)

#define CHECK_DISTANCE_PREDICATE(S1, S2, P3, RES) \
    BOOST_CHECK_EQUAL(distance_predicate(S1, S2, P3), RES)

#define CHECK_NODE_COMPARISON(node, nodes, res, sz) \
    for (int i = 0; i < sz; ++i) { \
      BOOST_CHECK_EQUAL(node_comparison(node, nodes[i]), res[i]); \
      BOOST_CHECK_EQUAL(node_comparison(nodes[i], node), !res[i]); \
    }

#define CHECK_CIRCLE(circle, c_x, c_y, l_x) \
    BOOST_CHECK_EQUAL(ulp_cmp(c1.x(), c_x, 10), ulp_comparison<double>::EQUAL); \
    BOOST_CHECK_EQUAL(ulp_cmp(c1.y(), c_y, 10), ulp_comparison<double>::EQUAL); \
    BOOST_CHECK_EQUAL(ulp_cmp(c1.lower_x(), l_x, 10), ulp_comparison<double>::EQUAL)

#define CHECK_CIRCLE_EXISTENCE(s1, s2, s3, RES) \
  { circle_type c1; \
    BOOST_CHECK_EQUAL(lazy_predicate(s1, s2, s3, c1), RES); }

#define CHECK_CIRCLE_FORMATION_PREDICATE(s1, s2, s3, c_x, c_y, l_x) \
  { circle_type c1, c2; \
    BOOST_CHECK_EQUAL(mp_predicate(s1, s2, s3, c1), true); \
    BOOST_CHECK_EQUAL(lazy_predicate(s1, s2, s3, c2), true); \
    CHECK_CIRCLE(c1, c_x, c_y, l_x); \
    CHECK_CIRCLE(c2, c_x, c_y, l_x); }

BOOST_AUTO_TEST_CASE(orientation_test) {
  int min_int = (std::numeric_limits<int>::min)();
  int max_int = (std::numeric_limits<int>::max)();
  point_type point1(min_int, min_int);
  point_type point2(0, 0);
  point_type point3(max_int, max_int);
  point_type point4(min_int, max_int);
  point_type point5(max_int-1, max_int);
  CHECK_ORIENTATION(point1, point2, point3, VP::ot::COLLINEAR, VP::ot::COLLINEAR);
  CHECK_ORIENTATION(point1, point4, point3, VP::ot::RIGHT, VP::ot::LEFT);
  CHECK_ORIENTATION(point1, point5, point3, VP::ot::RIGHT, VP::ot::LEFT);
}

BOOST_AUTO_TEST_CASE(event_comparison_test1) {
  site_type site(1, 2);
  CHECK_EVENT_COMPARISON(site, site_type(0, 2), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(1, 3), true, false);
  CHECK_EVENT_COMPARISON(site, site_type(1, 2), false, false);
}

BOOST_AUTO_TEST_CASE(event_comparison_test2) {
  site_type site(0, 0, 0, 2);
  CHECK_EVENT_COMPARISON(site, site_type(0, 2), true, false);
  CHECK_EVENT_COMPARISON(site, site_type(0, 0), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, -2, 0, -1), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, -2, 1, 1), true, false);
  CHECK_EVENT_COMPARISON(site, site_type(0, 0, 1, 1), true, false);
}

BOOST_AUTO_TEST_CASE(event_comparison_test3) {
  site_type site(0, 0, 10, 10);
  CHECK_EVENT_COMPARISON(site, site_type(0, 0), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, -1), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, 1), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, 1, 0, 10), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, -10, 0, -1), false, true);
  CHECK_EVENT_COMPARISON(site, site_type(0, 0, 10, 9), true, false);
  CHECK_EVENT_COMPARISON(site, site_type(0, 0, 9, 10), false, true);
}

BOOST_AUTO_TEST_CASE(event_comparison_test4) {
  circle_type circle(1, 2, 3);
  CHECK_EVENT_COMPARISON(circle, circle_type(1, 2, 3), false, false);
  CHECK_EVENT_COMPARISON(circle, circle_type(1, 3, 3), true, false);
  CHECK_EVENT_COMPARISON(circle, circle_type(1, 2, 4), true, false);
  CHECK_EVENT_COMPARISON(circle, circle_type(0, 2, 2), false, true);
  CHECK_EVENT_COMPARISON(circle, circle_type(-1, 2, 3), false, false);
}

BOOST_AUTO_TEST_CASE(event_comparison_test5) {
  circle_type circle(1, 2, 3);
  CHECK_EVENT_COMPARISON(circle, site_type(0, 100), false, true);
  CHECK_EVENT_COMPARISON(circle, site_type(3, 0), false, false);
  CHECK_EVENT_COMPARISON(circle, site_type(3, 2), false, false);
  CHECK_EVENT_COMPARISON(circle, site_type(3, 3), false, false);
  CHECK_EVENT_COMPARISON(circle, site_type(4, 2), true, false);
}

BOOST_AUTO_TEST_CASE(distance_predicate_test1) {
  site_type site1(-5, 0);
  site1.sorted_index(1);
  site_type site2(-8, 9);
  site2.sorted_index(0);
  site_type site3(-2, 1);
  site3.sorted_index(2);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 5), false);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, 5), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 4), false);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, 4), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 6), true);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, 6), true);
}

BOOST_AUTO_TEST_CASE(distance_predicate_test2) {
  site_type site1(-4, 0, -4, 20);
  site1.sorted_index(0);
  site_type site2(-2, 10);
  site2.sorted_index(1);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, 11), false);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, 9), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 11), true);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 9), true);
}

BOOST_AUTO_TEST_CASE(disntace_predicate_test3) {
  site_type site1(-5, 5, 2, -2);
  site1.sorted_index(0);
  site1.inverse();
  site_type site2(-2, 4);
  site2.sorted_index(1);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, -1), false);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, -1), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 1), false);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, 1), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 4), true);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, 4), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 5), true);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, 5), false);
}

BOOST_AUTO_TEST_CASE(distance_predicate_test4) {
  site_type site1(-5, 5, 2, -2);
  site1.sorted_index(0);
  site_type site2(-2, -4);
  site2.sorted_index(2);
  site_type site3(-4, 1);
  site3.sorted_index(1);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, 1), true);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, 1), true);
  CHECK_DISTANCE_PREDICATE(site1, site3, point_type(0, 1), true);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, 1), true);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, -2), true);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, -2), false);
  CHECK_DISTANCE_PREDICATE(site1, site3, point_type(0, -2), true);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, -2), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, -8), true);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, -8), false);
  CHECK_DISTANCE_PREDICATE(site1, site3, point_type(0, -8), true);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, -8), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(0, -9), true);
  CHECK_DISTANCE_PREDICATE(site2, site1, point_type(0, -9), false);
  CHECK_DISTANCE_PREDICATE(site1, site3, point_type(0, -9), true);
  CHECK_DISTANCE_PREDICATE(site3, site1, point_type(0, -9), false);
}

BOOST_AUTO_TEST_CASE(disntace_predicate_test5) {
  site_type site1(-5, 5, 2, -2);
  site1.sorted_index(0);
  site_type site2 = site1;
  site2.inverse();
  site_type site3(-2, 4);
  site3.sorted_index(3);
  site_type site4(-2, -4);
  site4.sorted_index(2);
  site_type site5(-4, 1);
  site5.sorted_index(1);
  CHECK_DISTANCE_PREDICATE(site3, site2, point_type(0, 1), false);
  CHECK_DISTANCE_PREDICATE(site3, site2, point_type(0, 4), false);
  CHECK_DISTANCE_PREDICATE(site3, site2, point_type(0, 5), false);
  CHECK_DISTANCE_PREDICATE(site3, site2, point_type(0, 7), true);
  CHECK_DISTANCE_PREDICATE(site4, site1, point_type(0, -2), false);
  CHECK_DISTANCE_PREDICATE(site5, site1, point_type(0, -2), false);
  CHECK_DISTANCE_PREDICATE(site4, site1, point_type(0, -8), false);
  CHECK_DISTANCE_PREDICATE(site5, site1, point_type(0, -8), false);
  CHECK_DISTANCE_PREDICATE(site4, site1, point_type(0, -9), false);
  CHECK_DISTANCE_PREDICATE(site5, site1, point_type(0, -9), false);
  CHECK_DISTANCE_PREDICATE(site4, site1, point_type(0, -18), false);
  CHECK_DISTANCE_PREDICATE(site5, site1, point_type(0, -18), false);
  CHECK_DISTANCE_PREDICATE(site4, site1, point_type(0, -1), true);
  CHECK_DISTANCE_PREDICATE(site5, site1, point_type(0, -1), true);
}

BOOST_AUTO_TEST_CASE(distance_predicate_test6) {
  site_type site1(-5, 0, 2, 7);
  site_type site2 = site1;
  site2.inverse();
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(2, 7), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(1, 5), false);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(-1, 5), true);
}

BOOST_AUTO_TEST_CASE(distance_predicate_test7) {
  site_type site1(-5, 5, 2, -2);
  site1.sorted_index(1);
  site1.inverse();
  site_type site2(-5, 5, 0, 6);
  site2.sorted_index(0);
  site_type site3(-2, 4, 0, 4);
  site3.sorted_index(2);
  point_type site4(0, 2);
  point_type site5(0, 5);
  point_type site6(0, 6);
  point_type site7(0, 8);
  CHECK_DISTANCE_PREDICATE(site1, site2, site4, false);
  CHECK_DISTANCE_PREDICATE(site1, site2, site5, true);
  CHECK_DISTANCE_PREDICATE(site1, site2, site6, true);
  CHECK_DISTANCE_PREDICATE(site1, site2, site7, true);
  CHECK_DISTANCE_PREDICATE(site1, site3, site4, false);
  CHECK_DISTANCE_PREDICATE(site1, site3, site5, true);
  CHECK_DISTANCE_PREDICATE(site1, site3, site6, true);
  CHECK_DISTANCE_PREDICATE(site1, site3, site7, true);
  site3.inverse();
  CHECK_DISTANCE_PREDICATE(site3, site1, site4, false);
  CHECK_DISTANCE_PREDICATE(site3, site1, site5, false);
  CHECK_DISTANCE_PREDICATE(site3, site1, site6, false);
  CHECK_DISTANCE_PREDICATE(site3, site1, site7, true);
}

BOOST_AUTO_TEST_CASE(distance_predicate_test8) {
  site_type site1(-5, 3, -2, 2);
  site1.sorted_index(0);
  site1.inverse();
  site_type site2(-5, 5, -2, 2);
  site2.sorted_index(1);
  CHECK_DISTANCE_PREDICATE(site1, site2, point_type(-4, 2), false);
}

BOOST_AUTO_TEST_CASE(node_comparison_test1) {
  beach_line_type beach_line;
  site_type site1(0, 0);
  site1.sorted_index(0);
  site_type site2(0, 2);
  site2.sorted_index(1);
  site_type site3(1, 0);
  site3.sorted_index(2);
  beach_line[key_type(site1, site2)] = 2;
  beach_line[key_type(site1, site3)] = 0;
  beach_line[key_type(site3, site1)] = 1;
  int cur_index = 0;
  for (bieach_line_iterator it = beach_line.begin();
       it != beach_line.end(); ++it, ++cur_index) {
    BOOST_CHECK_EQUAL(it->second, cur_index);
  }
}

BOOST_AUTO_TEST_CASE(node_comparison_test2) {
  beach_line_type beach_line;
  site_type site1(0, 1);
  site1.sorted_index(0);
  site_type site2(2, 0);
  site2.sorted_index(1);
  site_type site3(2, 4);
  site3.sorted_index(2);
  beach_line[key_type(site1, site2)] = 0;
  beach_line[key_type(site2, site1)] = 1;
  beach_line[key_type(site1, site3)] = 2;
  beach_line[key_type(site3, site1)] = 3;
  int cur_index = 0;
  for (bieach_line_iterator it = beach_line.begin();
       it != beach_line.end(); ++it, ++cur_index) {
    BOOST_CHECK_EQUAL(it->second, cur_index);
  }
}

BOOST_AUTO_TEST_CASE(node_comparison_test3) {
  key_type node(site_type(1, 0).sorted_index(1), site_type(0, 2).sorted_index(0));
  key_type nodes[] = {
    key_type(site_type(2, -10).sorted_index(2)),
    key_type(site_type(2, -1).sorted_index(2)),
    key_type(site_type(2, 0).sorted_index(2)),
    key_type(site_type(2, 1).sorted_index(2)),
    key_type(site_type(2, 2).sorted_index(2)),
    key_type(site_type(2, 3).sorted_index(2)),
  };
  bool res[] = {false, false, false, false, true, true};
  CHECK_NODE_COMPARISON(node, nodes, res, 6);
}

BOOST_AUTO_TEST_CASE(node_comparison_test4) {
  key_type node(site_type(0, 1).sorted_index(0), site_type(1, 0).sorted_index(1));
  key_type nodes[] = {
    key_type(site_type(2, -3).sorted_index(2)),
    key_type(site_type(2, -2).sorted_index(2)),
    key_type(site_type(2, -1).sorted_index(2)),
    key_type(site_type(2, 0).sorted_index(2)),
    key_type(site_type(2, 1).sorted_index(2)),
    key_type(site_type(2, 3).sorted_index(2)),
  };
  bool res[] = {false, true, true, true, true, true};
  CHECK_NODE_COMPARISON(node, nodes, res, 6);
}

BOOST_AUTO_TEST_CASE(node_comparison_test5) {
  key_type node(site_type(0, 0).sorted_index(0), site_type(1, 2).sorted_index(1));
  key_type nodes[] = {
    key_type(site_type(2, -10).sorted_index(2)),
    key_type(site_type(2, 0).sorted_index(2)),
    key_type(site_type(2, 1).sorted_index(2)),
    key_type(site_type(2, 2).sorted_index(2)),
    key_type(site_type(2, 5).sorted_index(2)),
    key_type(site_type(2, 20).sorted_index(2)),
  };
  bool res[] = {false, false, true, true, true, true};
  CHECK_NODE_COMPARISON(node, nodes, res, 6);
}

BOOST_AUTO_TEST_CASE(node_comparison_test6) {
  key_type node(site_type(1, 1).sorted_index(1), site_type(0, 0).sorted_index(0));
  key_type nodes[] = {
    key_type(site_type(2, -3).sorted_index(2)),
    key_type(site_type(2, -2).sorted_index(2)),
    key_type(site_type(2, 0).sorted_index(2)),
    key_type(site_type(2, 1).sorted_index(2)),
    key_type(site_type(2, 2).sorted_index(2)),
    key_type(site_type(2, 3).sorted_index(2)),
    key_type(site_type(2, 5).sorted_index(2)),
  };
  bool res[] = {false, false, false, false, false, false, true};
  CHECK_NODE_COMPARISON(node, nodes, res, 7);
}

BOOST_AUTO_TEST_CASE(node_comparison_test7) {
  key_type node(site_type(0, 0).sorted_index(0), site_type(0, 2).sorted_index(1));
  key_type nodes[] = {
    key_type(site_type(1, 0).sorted_index(2)),
    key_type(site_type(1, 1).sorted_index(2)),
    key_type(site_type(1, 2).sorted_index(2)),
  };
  bool res[] = {false, false, true};
  CHECK_NODE_COMPARISON(node, nodes, res, 3);
}

BOOST_AUTO_TEST_CASE(node_comparison_test8) {
  key_type node(site_type(0, 0).sorted_index(0), site_type(1, 1).sorted_index(2));
  key_type nodes[] = {
    key_type(site_type(1, 0).sorted_index(1)),
    key_type(site_type(1, 1).sorted_index(2)),
    key_type(site_type(1, 2).sorted_index(3)),
    key_type(site_type(1, 1).sorted_index(2), site_type(0, 0).sorted_index(0)),
  };
  bool res[] = {false, true, true, true};
  CHECK_NODE_COMPARISON(node, nodes, res, 4);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test1) {
  site_type site1(0, 0);
  site1.sorted_index(1);
  site_type site2(-8, 0);
  site2.sorted_index(0);
  site_type site3(0, 6);
  site3.sorted_index(2);
  CHECK_CIRCLE_FORMATION_PREDICATE(site1, site2, site3, -4.0, 3.0, 1.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test2) {
  int min_int = (std::numeric_limits<int>::min)();
  int max_int = (std::numeric_limits<int>::max)();
  site_type site1(min_int, min_int);
  site1.sorted_index(0);
  site_type site2(min_int, max_int);
  site2.sorted_index(1);
  site_type site3(max_int-1, max_int-1);
  site3.sorted_index(2);
  site_type site4(max_int, max_int);
  site4.sorted_index(3);
  CHECK_CIRCLE_EXISTENCE(site1, site2, site4, true);
  CHECK_CIRCLE_EXISTENCE(site1, site3, site4, false);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test3) {
  site_type site1(-4, 0);
  site1.sorted_index(0);
  site_type site2(0, 4);
  site2.sorted_index(4);
  site_type site3(site1.point0(), site2.point0());
  site3.sorted_index(1);
  CHECK_CIRCLE_EXISTENCE(site1, site3, site2, false);
  site_type site4(-2, 0);
  site4.sorted_index(2);
  site_type site5(0, 2);
  site5.sorted_index(3);
  CHECK_CIRCLE_EXISTENCE(site3, site4, site5, false);
  CHECK_CIRCLE_EXISTENCE(site4, site5, site3, false);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test4) {
  site_type site1(-4, 0, -4, 20);
  site1.sorted_index(0);
  site_type site2(-2, 10);
  site2.sorted_index(1);
  site_type site3(4, 10);
  site3.sorted_index(2);
  CHECK_CIRCLE_FORMATION_PREDICATE(site1, site2, site3, 1.0, 6.0, 6.0);
  CHECK_CIRCLE_FORMATION_PREDICATE(site3, site2, site1, 1.0, 14.0, 6.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test5) {
  site_type site1(1, 0, 7, 0);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(-2, 4, 10, 4);
  site2.sorted_index(0);
  site_type site3(6, 2);
  site3.sorted_index(3);
  site_type site4(1, 0);
  site4.sorted_index(1);
  CHECK_CIRCLE_FORMATION_PREDICATE(site3, site1, site2, 4.0, 2.0, 6.0);
  CHECK_CIRCLE_FORMATION_PREDICATE(site4, site2, site1, 1.0, 2.0, 3.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test6) {
  site_type site1(-1, 2, 8, -10);
  site1.sorted_index(1);
  site1.inverse();
  site_type site2(-1, 0, 8, 12);
  site2.sorted_index(0);
  site_type site3(1, 1);
  site3.sorted_index(2);
  CHECK_CIRCLE_FORMATION_PREDICATE(site3, site2, site1, 6.0, 1.0, 11.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test7) {
  site_type site1(1, 0, 6, 0);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(-6, 4, 0, 12);
  site2.sorted_index(0);
  site_type site3(1, 0);
  site3.sorted_index(1);
  CHECK_CIRCLE_FORMATION_PREDICATE(site3, site2, site1, 1.0, 5.0, 6.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test8) {
  site_type site1(1, 0, 5, 0);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(0, 12, 8, 6);
  site2.sorted_index(0);
  site_type site3(1, 0);
  site3.sorted_index(1);
  CHECK_CIRCLE_FORMATION_PREDICATE(site3, site2, site1, 1.0, 5.0, 6.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test9) {
  site_type site1(0, 0, 4, 0);
  site1.sorted_index(1);
  site_type site2(0, 0, 0, 4);
  site2.sorted_index(0);
  site_type site3(0, 4, 4, 4);
  site3.sorted_index(2);
  site1.inverse();
  CHECK_CIRCLE_FORMATION_PREDICATE(site1, site2, site3, 2.0, 2.0, 4.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test10) {
  site_type site1(1, 0, 41, 30);
  site1.sorted_index(1);
  site_type site2(-39, 30, 1, 60);
  site2.sorted_index(0);
  site_type site3(1, 60, 41, 30);
  site3.sorted_index(2);
  site1.inverse();
  CHECK_CIRCLE_FORMATION_PREDICATE(site1, site2, site3, 1.0, 30.0, 25.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test11) {
  site_type site1(0, 0, 0, 10);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(-8, 10);
  site2.sorted_index(0);
  site_type site3(-7, 14, -1, 14);
  site3.sorted_index(1);
  CHECK_CIRCLE_FORMATION_PREDICATE(site1, site2, site3, -4.0, 10.0, 0.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test12) {
  site_type site1(0, 0, 0, 10);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(-8, 10);
  site2.sorted_index(0);
  site_type site3(-7, 15, -1, 15);
  site3.sorted_index(1);
  CHECK_CIRCLE_EXISTENCE(site1, site2, site3, false);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test13) {
  site_type site1(0, 0, 0, 10);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(-7, -4, -1, -4);
  site2.sorted_index(1);
  site2.inverse();
  site_type site3(-8, 0);
  site3.sorted_index(0);
  CHECK_CIRCLE_FORMATION_PREDICATE(site1, site2, site3, -4.0, 0.0, 0.0);
}

BOOST_AUTO_TEST_CASE(circle_formation_predicate_test14) {
  site_type site1(0, 0, 0, 10);
  site1.sorted_index(2);
  site1.inverse();
  site_type site2(-7, -5, -1, -5);
  site2.sorted_index(1);
  site2.inverse();
  site_type site3(-8, 0);
  site3.sorted_index(0);
  CHECK_CIRCLE_EXISTENCE(site1, site2, site3, false);
}
