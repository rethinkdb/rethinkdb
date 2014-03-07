// Boost.Polygon library polygon_point_test.cpp file

//          Copyright Andrii Sydorchuk 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#define BOOST_TEST_MODULE POLYGON_POINT_TEST
#include <boost/mpl/list.hpp>
#include <boost/test/test_case_template.hpp>

#include "boost/polygon/point_concept.hpp"
#include "boost/polygon/point_data.hpp"
#include "boost/polygon/point_traits.hpp"
using namespace boost::polygon;

typedef boost::mpl::list<int> test_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(point_data_test, T, test_types) {
  typedef point_data<T> point_type;

  point_type point1(1, 2);
  point_type point2;
  point2 = point1;
  BOOST_CHECK_EQUAL(point1.x(), 1);
  BOOST_CHECK_EQUAL(point1.y(), 2);
  BOOST_CHECK_EQUAL(point2.x(), 1);
  BOOST_CHECK_EQUAL(point2.y(), 2);
  BOOST_CHECK(point1 == point2);
  BOOST_CHECK(!(point1 != point2));
  BOOST_CHECK(!(point1 < point2));
  BOOST_CHECK(!(point1 > point2));
  BOOST_CHECK(point1 <= point2);
  BOOST_CHECK(point1 >= point2);

  point2.x(2);
  point2.y(1);
  BOOST_CHECK_EQUAL(point2.x(), 2);
  BOOST_CHECK_EQUAL(point2.y(), 1);
  BOOST_CHECK(!(point1 == point2));
  BOOST_CHECK(point1 != point2);
  BOOST_CHECK(point1 < point2);
  BOOST_CHECK(!(point1 > point2));
  BOOST_CHECK(point1 <= point2);
  BOOST_CHECK(!(point1 >= point2));

  point2.set(HORIZONTAL, 1);
  point2.set(VERTICAL, 2);
  BOOST_CHECK(point1 == point2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(point_traits_test, T, test_types) {
  typedef point_data<T> point_type;

  point_type point = point_mutable_traits<point_type>::construct(1, 2);
  BOOST_CHECK_EQUAL(point_traits<point_type>::get(point, HORIZONTAL), 1);
  BOOST_CHECK_EQUAL(point_traits<point_type>::get(point, VERTICAL), 2);

  point_mutable_traits<point_type>::set(point, HORIZONTAL, 3);
  point_mutable_traits<point_type>::set(point, VERTICAL, 4);
  BOOST_CHECK_EQUAL(point_traits<point_type>::get(point, HORIZONTAL), 3);
  BOOST_CHECK_EQUAL(point_traits<point_type>::get(point, VERTICAL), 4);
}

template <typename T>
struct Point {
  T x;
  T y;
};

namespace boost {
namespace polygon {
  template <typename T>
  struct geometry_concept< Point<T> > {
    typedef point_concept type;
  };

  template <typename T>
  struct point_traits< Point<T> > {
    typedef T coordinate_type;

    static coordinate_type get(const Point<T>& point, orientation_2d orient) {
      return (orient == HORIZONTAL) ? point.x : point.y;
    }
  };

  template <typename T>
  struct point_mutable_traits< Point<T> > {
    typedef T coordinate_type;

    static void set(Point<T>& point, orientation_2d orient, T value) {
      (orient == HORIZONTAL) ? point.x = value : point.y = value;
    }

    static Point<T> construct(coordinate_type x, coordinate_type y) {
      Point<T> point;
      point.x = x;
      point.y = y;
      return point;
    }
  };
}  // polygon
}  // boost

BOOST_AUTO_TEST_CASE_TEMPLATE(point_concept_test1, T, test_types) {
  typedef Point<T> point_type;

  point_type point1 = construct<point_type>(1, 2);
  BOOST_CHECK_EQUAL(point1.x, 1);
  BOOST_CHECK_EQUAL(point1.y, 2);

  set(point1, HORIZONTAL, 3);
  set(point1, VERTICAL, 4);
  BOOST_CHECK_EQUAL(get(point1, HORIZONTAL), 3);
  BOOST_CHECK_EQUAL(get(point1, VERTICAL), 4);

  point_type point2;
  assign(point2, point1);
  BOOST_CHECK(equivalence(point1, point2));

  x(point2, 1);
  y(point2, 2);
  BOOST_CHECK_EQUAL(x(point2), 1);
  BOOST_CHECK_EQUAL(y(point2), 2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(point_concept_test2, T, test_types) {
  typedef Point<T> point_type;

  point_type point1 = construct<point_type>(1, 2);
  point_type point2 = construct<point_type>(5, 5);
  BOOST_CHECK_EQUAL(euclidean_distance(point1, point2, HORIZONTAL), 4);
  BOOST_CHECK_EQUAL(euclidean_distance(point1, point2, VERTICAL), 3);
  BOOST_CHECK_EQUAL(manhattan_distance(point1, point2), 7);
  BOOST_CHECK_EQUAL(euclidean_distance(point1, point2), 5.0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(point_concept_test3, T, test_types) {
  typedef Point<T> point_type;

  point_type point = construct<point_type>(1, 2);
  point_type shift = construct<point_type>(4, 3);
  convolve(point, shift);
  BOOST_CHECK_EQUAL(x(point), 5);
  BOOST_CHECK_EQUAL(y(point), 5);

  deconvolve(point, shift);
  BOOST_CHECK_EQUAL(x(point), 1);
  BOOST_CHECK_EQUAL(y(point), 2);

  scale_up(point, 5);
  BOOST_CHECK_EQUAL(x(point), 5);
  BOOST_CHECK_EQUAL(y(point), 10);

  scale_down(point, 5);
  BOOST_CHECK_EQUAL(x(point), 1);
  BOOST_CHECK_EQUAL(y(point), 2);

  move(point, HORIZONTAL, 2);
  move(point, VERTICAL, 3);
  BOOST_CHECK_EQUAL(x(point), 3);
  BOOST_CHECK_EQUAL(y(point), 5);
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

BOOST_AUTO_TEST_CASE_TEMPLATE(point_concept_test4, T, test_types) {
  typedef Point<T> point_type;

  point_type point = construct<point_type>(1, 2);
  scale(point, Transformer<T>());
  BOOST_CHECK_EQUAL(x(point), 2);
  BOOST_CHECK_EQUAL(y(point), 4);

  transform(point, Transformer<T>());
  BOOST_CHECK_EQUAL(x(point), 4);
  BOOST_CHECK_EQUAL(y(point), 2);
}
