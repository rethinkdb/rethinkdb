// Boost.Polygon library polygon_segment_test.cpp file

//          Copyright Andrii Sydorchuk 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#define BOOST_TEST_MODULE POLYGON_SEGMENT_TEST
#include <boost/mpl/list.hpp>
#include <boost/test/test_case_template.hpp>

#include "boost/polygon/segment_concept.hpp"
#include "boost/polygon/segment_data.hpp"
#include "boost/polygon/segment_traits.hpp"
using namespace boost::polygon;

typedef boost::mpl::list<int> test_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_data_test, T, test_types) {
  typedef point_data<T> point_type;
  typedef segment_data<T> segment_type;
  point_type point1(1, 2);
  point_type point2(3, 4);
  segment_type segment1(point1, point2);
  segment_type segment2;
  segment2 = segment1;

  BOOST_CHECK(segment1.low() == point1);
  BOOST_CHECK(segment1.high() == point2);
  BOOST_CHECK(segment1.get(LOW) == point1);
  BOOST_CHECK(segment1.get(HIGH) == point2);
  BOOST_CHECK(segment1 == segment2);
  BOOST_CHECK(!(segment1 != segment2));
  BOOST_CHECK(!(segment1 < segment2));
  BOOST_CHECK(!(segment1 > segment1));
  BOOST_CHECK(segment1 <= segment2);
  BOOST_CHECK(segment1 >= segment2);

  segment1.low(point2);
  segment1.high(point1);
  BOOST_CHECK(segment1.low() == point2);
  BOOST_CHECK(segment1.high() == point1);
  BOOST_CHECK(!(segment1 == segment2));
  BOOST_CHECK(segment1 != segment2);

  segment2.set(LOW, point2);
  segment2.set(HIGH, point1);
  BOOST_CHECK(segment1 == segment2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_traits_test, T, test_types) {
  typedef point_data<T> point_type;
  typedef segment_data<T> segment_type;

  point_type point1(1, 2);
  point_type point2(3, 4);
  segment_type segment =
      segment_mutable_traits<segment_type>::construct(point1, point2);

  BOOST_CHECK(segment_traits<segment_type>::get(segment, LOW) == point1);
  BOOST_CHECK(segment_traits<segment_type>::get(segment, HIGH) == point2);

  segment_mutable_traits<segment_type>::set(segment, LOW, point2);
  segment_mutable_traits<segment_type>::set(segment, HIGH, point1);
  BOOST_CHECK(segment_traits<segment_type>::get(segment, LOW) == point2);
  BOOST_CHECK(segment_traits<segment_type>::get(segment, HIGH) == point1);
}

template <typename T>
struct Segment {
  typedef T coordinate_type;
  typedef point_data<T> point_type;
  point_type p0;
  point_type p1;
};

namespace boost {
namespace polygon {
  template <typename T>
  struct geometry_concept< Segment<T> > {
    typedef segment_concept type;
  };

  template <typename T>
  struct segment_traits< Segment<T> > {
    typedef T coordinate_type;
    typedef point_data<T> point_type;

    static point_type get(const Segment<T>& segment, direction_1d dir) {
      return dir.to_int() ? segment.p1 : segment.p0;
    }
  };

  template <typename T>
  struct segment_mutable_traits< Segment<T> > {
    typedef T coordinate_type;
    typedef point_data<T> point_type;

    static void set(
        Segment<T>& segment, direction_1d dir, const point_type& point) {
      dir.to_int() ? segment.p1 = point : segment.p0 = point;;
    }

    static Segment<T> construct(
        const point_type& point1, const point_type& point2) {
      Segment<T> segment;
      segment.p0 = point1;
      segment.p1 = point2;
      return segment;
    }
  };
}
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test1, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(1, 2);
  point_type point2(3, 4);
  point_type point3(2, 3);
  segment_type segment1 = construct<segment_type>(point1, point2);
  BOOST_CHECK(segment1.p0 == point1);
  BOOST_CHECK(segment1.p1 == point2);
  BOOST_CHECK(get(segment1, LOW) == point1);
  BOOST_CHECK(low(segment1) == point1);
  BOOST_CHECK(get(segment1, HIGH) == point2);
  BOOST_CHECK(high(segment1) == point2);
  BOOST_CHECK(center(segment1) == point3);

  set(segment1, LOW, point2);
  set(segment1, HIGH, point1);
  BOOST_CHECK(segment1.p0 == point2);
  BOOST_CHECK(segment1.p1 == point1);
  BOOST_CHECK(get(segment1, LOW) == point2);
  BOOST_CHECK(get(segment1, HIGH) == point1);
  low(segment1, point1);
  high(segment1, point2);
  BOOST_CHECK(segment1.p0 == point1);
  BOOST_CHECK(segment1.p1 == point2);

  segment_data<T> segment2 = copy_construct< segment_data<T> >(segment1);
  BOOST_CHECK(segment1.p0 == segment2.low());
  BOOST_CHECK(segment1.p1 == segment2.high());
  BOOST_CHECK(equivalence(segment1, segment2));

  segment_data<T> segment3 = construct< segment_data<T> >(point2, point1);
  assign(segment1, segment3);
  BOOST_CHECK(segment1.p0 == point2);
  BOOST_CHECK(segment1.p1 == point1);
  BOOST_CHECK(!equivalence(segment1, segment2));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test2, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(1, 2);
  point_type point2(2, 4);
  point_type point3(0, 0);
  point_type point4(5, 10);
  point_type point5(1, 3);
  point_type point6(2, 3);
  point_type point7(100, 201);
  point_type point8(100, 200);
  point_type point9(100, 199);
  segment_type segment1 = construct<segment_type>(point1, point2);
  segment_type segment2 = construct<segment_type>(point2, point1);
  segment_type segment3 = construct<segment_type>(point1, point5);

  BOOST_CHECK(orientation(segment1, point1) == 0);
  BOOST_CHECK(orientation(segment1, point2) == 0);
  BOOST_CHECK(orientation(segment1, point3) == 0);
  BOOST_CHECK(orientation(segment1, point4) == 0);
  BOOST_CHECK(orientation(segment1, point5) == 1);
  BOOST_CHECK(orientation(segment2, point5) == -1);
  BOOST_CHECK(orientation(segment1, point6) == -1);
  BOOST_CHECK(orientation(segment2, point6) == 1);
  BOOST_CHECK(orientation(segment1, point7) == 1);
  BOOST_CHECK(orientation(segment2, point7) == -1);
  BOOST_CHECK(orientation(segment1, point8) == 0);
  BOOST_CHECK(orientation(segment1, point9) == -1);
  BOOST_CHECK(orientation(segment2, point9) == 1);
  BOOST_CHECK(orientation(segment3, point6) == -1);
  BOOST_CHECK(orientation(segment3, point3) == 1);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test3, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  segment_type segment1 = construct<segment_type>(
      point_type(0, 0), point_type(1, 2));
  segment_type segment2 = construct<segment_type>(
      point_type(0, 0), point_type(2, 4));
  segment_type segment3 = construct<segment_type>(
      point_type(0, 0), point_type(2, 3));
  segment_type segment4 = construct<segment_type>(
      point_type(0, 0), point_type(2, 5));
  segment_type segment5 = construct<segment_type>(
      point_type(0, 2), point_type(2, 0));

  BOOST_CHECK(orientation(segment1, segment2) == 0);
  BOOST_CHECK(orientation(segment1, segment3) == -1);
  BOOST_CHECK(orientation(segment3, segment1) == 1);
  BOOST_CHECK(orientation(segment1, segment4) == 1);
  BOOST_CHECK(orientation(segment4, segment1) == -1);
  BOOST_CHECK(orientation(segment1, segment5) == -1);
  BOOST_CHECK(orientation(segment5, segment1) == 1);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test4, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(1, 2);
  point_type point2(3, 6);
  point_type point3(2, 4);
  point_type point4(4, 8);
  point_type point5(0, 0);
  segment_type segment = construct<segment_type>(point1, point2);

  BOOST_CHECK(contains(segment, point1, true));
  BOOST_CHECK(contains(segment, point2, true));
  BOOST_CHECK(!contains(segment, point1, false));
  BOOST_CHECK(!contains(segment, point2, false));
  BOOST_CHECK(contains(segment, point3, false));
  BOOST_CHECK(!contains(segment, point4, true));
  BOOST_CHECK(!contains(segment, point5, true));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test5, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(0, 0);
  point_type point2(10, 0);
  point_type point3(5, 0);
  point_type point4(-1, 0);
  point_type point5(11, 0);
  segment_type segment = construct<segment_type>(point1, point2);

  BOOST_CHECK(contains(segment, point1, true));
  BOOST_CHECK(contains(segment, point2, true));
  BOOST_CHECK(!contains(segment, point1, false));
  BOOST_CHECK(!contains(segment, point2, false));
  BOOST_CHECK(contains(segment, point3, false));
  BOOST_CHECK(!contains(segment, point4, true));
  BOOST_CHECK(!contains(segment, point5, true));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test6, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(0, 0);
  point_type point2(1, 2);
  point_type point3(2, 4);
  point_type point4(3, 6);
  point_type point5(4, 8);
  point_type point6(5, 10);
  segment_type segment1 = construct<segment_type>(point2, point5);
  segment_type segment2 = construct<segment_type>(point3, point4);
  segment_type segment3 = construct<segment_type>(point1, point3);
  segment_type segment4 = construct<segment_type>(point4, point6);

  BOOST_CHECK(contains(segment1, segment2, false));
  BOOST_CHECK(!contains(segment2, segment1, true));
  BOOST_CHECK(!contains(segment1, segment3, true));
  BOOST_CHECK(!contains(segment1, segment4, true));
  BOOST_CHECK(contains(segment1, segment1, true));
  BOOST_CHECK(!contains(segment1, segment1, false));
}

template<typename T>
struct Transformer {
  void scale(T& x, T& y) const {
    x *= 2;
    y *= 2;
  }

  void transform(T& x, T& y) const {
    T tmp = x;
    x = y;
    y = tmp;
  }
};

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test7, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(1, 2);
  point_type point2(4, 6);
  segment_type segment1 = construct<segment_type>(point1, point2);

  scale_up(segment1, 3);
  BOOST_CHECK(low(segment1) == point_type(3, 6));
  BOOST_CHECK(high(segment1) == point_type(12, 18));

  scale_down(segment1, 3);
  BOOST_CHECK(low(segment1) == point1);
  BOOST_CHECK(high(segment1) == point2);
  BOOST_CHECK(length(segment1) == 5);

  move(segment1, HORIZONTAL, 1);
  move(segment1, VERTICAL, 2);
  BOOST_CHECK(low(segment1) == point_type(2, 4));
  BOOST_CHECK(high(segment1) == point_type(5, 8));
  BOOST_CHECK(length(segment1) == 5);

  convolve(segment1, point_type(1, 2));
  BOOST_CHECK(low(segment1) == point_type(3, 6));
  BOOST_CHECK(high(segment1) == point_type(6, 10));

  deconvolve(segment1, point_type(2, 4));
  BOOST_CHECK(low(segment1) == point1);
  BOOST_CHECK(high(segment1) == point2);

  scale(segment1, Transformer<T>());
  BOOST_CHECK(low(segment1) == point_type(2, 4));
  BOOST_CHECK(high(segment1) == point_type(8, 12));
  transform(segment1, Transformer<T>());
  BOOST_CHECK(low(segment1) == point_type(4, 2));
  BOOST_CHECK(high(segment1) == point_type(12, 8));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test8, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  segment_type segment1 = construct<segment_type>(
      point_type(0, 0), point_type(1, 2));
  segment_type segment2 = construct<segment_type>(
      point_type(1, 2), point_type(2, 4));
  segment_type segment3 = construct<segment_type>(
      point_type(2, 4), point_type(0, 4));
  segment_type segment4 = construct<segment_type>(
      point_type(0, 4), point_type(0, 0));

  BOOST_CHECK(abuts(segment1, segment2, HIGH));
  BOOST_CHECK(abuts(segment2, segment3, HIGH));
  BOOST_CHECK(abuts(segment3, segment4, HIGH));
  BOOST_CHECK(abuts(segment4, segment1, HIGH));

  BOOST_CHECK(!abuts(segment1, segment2, LOW));
  BOOST_CHECK(!abuts(segment2, segment3, LOW));
  BOOST_CHECK(!abuts(segment3, segment4, LOW));
  BOOST_CHECK(!abuts(segment4, segment1, LOW));

  BOOST_CHECK(abuts(segment2, segment1));
  BOOST_CHECK(abuts(segment3, segment2));
  BOOST_CHECK(abuts(segment4, segment3));
  BOOST_CHECK(abuts(segment1, segment4));

  BOOST_CHECK(!abuts(segment1, segment3));
  BOOST_CHECK(!abuts(segment2, segment4));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test9, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  segment_type segment1 = construct<segment_type>(
      point_type(0, 0), point_type(2, 2));
  segment_type segment2 = construct<segment_type>(
      point_type(1, 1), point_type(3, 3));
  segment_type segment3 = construct<segment_type>(
      point_type(2, 2), point_type(-1, -1));
  segment_type segment4 = construct<segment_type>(
      point_type(1, 3), point_type(3, 1));
  segment_type segment5 = construct<segment_type>(
      point_type(2, 2), point_type(1, 3));

  BOOST_CHECK(intersects(segment1, segment2, false));
  BOOST_CHECK(intersects(segment1, segment2, true));
  BOOST_CHECK(intersects(segment1, segment3, false));
  BOOST_CHECK(intersects(segment1, segment3, true));
  BOOST_CHECK(intersects(segment2, segment3, false));
  BOOST_CHECK(intersects(segment2, segment3, true));
  BOOST_CHECK(intersects(segment4, segment3, false));
  BOOST_CHECK(intersects(segment4, segment3, true));
  BOOST_CHECK(intersects(segment4, segment2, false));
  BOOST_CHECK(intersects(segment4, segment2, true));
  BOOST_CHECK(!intersects(segment3, segment5, false));
  BOOST_CHECK(intersects(segment3, segment5, true));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test10, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  segment_type segment1 = construct<segment_type>(
      point_type(0, 0), point_type(0, 2));
  segment_type segment2 = construct<segment_type>(
      point_type(0, 1), point_type(0, 3));
  segment_type segment3 = construct<segment_type>(
      point_type(0, 1), point_type(0, 2));
  segment_type segment4 = construct<segment_type>(
      point_type(0, 2), point_type(0, 3));
  segment_type segment5 = construct<segment_type>(
      point_type(0, 2), point_type(2, 2));
  segment_type segment6 = construct<segment_type>(
      point_type(0, 1), point_type(1, 1));

  BOOST_CHECK(intersects(segment1, segment1, false));
  BOOST_CHECK(intersects(segment1, segment1, true));
  BOOST_CHECK(intersects(segment1, segment2, false));
  BOOST_CHECK(intersects(segment1, segment2, true));
  BOOST_CHECK(intersects(segment1, segment3, false));
  BOOST_CHECK(intersects(segment1, segment3, true));
  BOOST_CHECK(intersects(segment2, segment3, false));
  BOOST_CHECK(intersects(segment2, segment3, true));
  BOOST_CHECK(!intersects(segment1, segment4, false));
  BOOST_CHECK(intersects(segment1, segment4, true));
  BOOST_CHECK(!intersects(segment1, segment5, false));
  BOOST_CHECK(intersects(segment1, segment5, true));
  BOOST_CHECK(intersects(segment1, segment6, false));
  BOOST_CHECK(intersects(segment1, segment6, true));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test11, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  point_type point1(1, 2);
  point_type point2(7, 10);
  segment_type segment1 = construct<segment_type>(point1, point2);

  BOOST_CHECK(euclidean_distance(segment1, point1) == 0.0);
  BOOST_CHECK(euclidean_distance(segment1, point2) == 0.0);
  BOOST_CHECK(euclidean_distance(segment1, point_type(10, 14)) == 5.0);
  BOOST_CHECK(euclidean_distance(segment1, point_type(-3, -1)) == 5.0);
  BOOST_CHECK(euclidean_distance(segment1, point_type(0, 9)) == 5.0);
  BOOST_CHECK(euclidean_distance(segment1, point_type(8, 3)) == 5.0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_concept_test12, T, test_types) {
  typedef point_data<T> point_type;
  typedef Segment<T> segment_type;

  segment_type segment1 = construct<segment_type>(
      point_type(0, 0), point_type(3, 4));
  segment_type segment2 = construct<segment_type>(
      point_type(2, 0), point_type(0, 2));
  segment_type segment3 = construct<segment_type>(
      point_type(1, -7), point_type(10, 5));
  segment_type segment4 = construct<segment_type>(
      point_type(7, 7), point_type(10, 11));

  BOOST_CHECK(euclidean_distance(segment1, segment2) == 0.0);
  BOOST_CHECK(euclidean_distance(segment1, segment3) == 5.0);
  BOOST_CHECK(euclidean_distance(segment1, segment4) == 5.0);
}
