/*==============================================================================
    Copyright (c) 2010-2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <ios>
#include <boost/config.hpp>
#include <boost/array.hpp>
#include <boost/detail/is_sorted.hpp>
#include <boost/detail/lightweight_test.hpp>

template<class T>
struct tracking_less: std::binary_function <T, T, bool> {
  typedef bool result_type;

  #if defined(__PATHSCALE__)
  tracking_less (void) { }
  ~tracking_less (void) { }
  #endif

  bool operator() (T const& x, T const& y) const {
    std::cout << x << " < " << y << " == " << (x < y) << "\n"; 
    return x < y;
  }
};

template<class T>
struct tracking_less_equal: std::binary_function <T, T, bool> {
  typedef bool result_type;

  #if defined(__PATHSCALE__)
  tracking_less_equal (void) { }
  ~tracking_less_equal (void) { }
  #endif

  bool operator() (T const& x, T const& y) const {
    std::cout << x << " <= " << y << " == " << (x <= y) << "\n"; 
    return x <= y;
  }
};

template<class T>
struct tracking_greater: std::binary_function <T, T, bool> {
  typedef bool result_type;

  #if defined(__PATHSCALE__)
  tracking_greater (void) { }
  ~tracking_greater (void) { }
  #endif

  bool operator() (T const& x, T const& y) const {
    std::cout << x << " > " << y << " == " << (x > y) << "\n"; 
    return x > y;
  }
};

template<class T>
struct tracking_greater_equal: std::binary_function <T, T, bool> {
  typedef bool result_type;

  #if defined(__PATHSCALE__)
  tracking_greater_equal (void) { }
  ~tracking_greater_equal (void) { }
  #endif

  bool operator() (T const& x, T const& y) const {
    std::cout << x << " >= " << y << " == " << (x >= y) << "\n"; 
    return x >= y;
  }
};


int main (void) {
  #define IS_SORTED ::boost::detail::is_sorted
  #define IS_SORTED_UNTIL ::boost::detail::is_sorted_until
  using boost::array;
  using boost::report_errors;

  std::cout << std::boolalpha;

  array<int, 10> a = { { 0, 1, 2,  3,  4, 5, 6,  7,  8,   9  } };
  array<int, 10> b = { { 0, 1, 1,  2,  5, 8, 13, 34, 55,  89 } };
  array<int, 10> c = { { 0, 1, -1, 2, -3, 5, -8, 13, -21, 34 } };

  tracking_less<int> lt;
  tracking_less_equal<int> lte;
  tracking_greater<int> gt;
  tracking_greater_equal<int> gte;

  BOOST_TEST_EQ(IS_SORTED_UNTIL(a.begin(), a.end()),          a.end());
  BOOST_TEST_EQ(IS_SORTED_UNTIL(a.begin(), a.end(), lt),      a.end());
  BOOST_TEST_EQ(IS_SORTED_UNTIL(a.begin(), a.end(), lte),     a.end());
  BOOST_TEST_EQ(*IS_SORTED_UNTIL(a.rbegin(), a.rend(), gt),   *a.rend());
  BOOST_TEST_EQ(*IS_SORTED_UNTIL(a.rbegin(), a.rend(), gte),  *a.rend());
  
  BOOST_TEST_EQ(IS_SORTED(a.begin(), a.end()),        true);
  BOOST_TEST_EQ(IS_SORTED(a.begin(), a.end(), lt),    true);
  BOOST_TEST_EQ(IS_SORTED(a.begin(), a.end(), lte),   true);
  BOOST_TEST_EQ(IS_SORTED(a.rbegin(), a.rend(), gt),  true);
  BOOST_TEST_EQ(IS_SORTED(a.rbegin(), a.rend(), gte), true);

  BOOST_TEST_EQ(IS_SORTED_UNTIL(b.begin(), b.end()),          b.end());
  BOOST_TEST_EQ(IS_SORTED_UNTIL(b.begin(), b.end(), lt),      b.end());
  BOOST_TEST_EQ(IS_SORTED_UNTIL(b.begin(), b.end(), lte),     &b[2]);
  BOOST_TEST_EQ(*IS_SORTED_UNTIL(b.rbegin(), b.rend(), gt),   *b.rend());
  BOOST_TEST_EQ(*IS_SORTED_UNTIL(b.rbegin(), b.rend(), gte),  b[2]);
  
  BOOST_TEST_EQ(IS_SORTED(b.begin(), b.end()),        true); 
  BOOST_TEST_EQ(IS_SORTED(b.begin(), b.end(), lt),    true); 
  BOOST_TEST_EQ(IS_SORTED(b.begin(), b.end(), lte),   false); 
  BOOST_TEST_EQ(IS_SORTED(b.rbegin(), b.rend(), gt),  true);
  BOOST_TEST_EQ(IS_SORTED(b.rbegin(), b.rend(), gte), false); 

  BOOST_TEST_EQ(IS_SORTED_UNTIL(c.begin(), c.end()),          &c[2]);
  BOOST_TEST_EQ(IS_SORTED_UNTIL(c.begin(), c.end(), lt),      &c[2]);
  BOOST_TEST_EQ(IS_SORTED_UNTIL(c.begin(), c.end(), lte),     &c[2]);
  BOOST_TEST_EQ(*IS_SORTED_UNTIL(c.rbegin(), c.rend(), gt),   c[7]);
  BOOST_TEST_EQ(*IS_SORTED_UNTIL(c.rbegin(), c.rend(), gte),  c[7]);
  
  BOOST_TEST_EQ(IS_SORTED(c.begin(), c.end()),        false);
  BOOST_TEST_EQ(IS_SORTED(c.begin(), c.end(), lt),    false);
  BOOST_TEST_EQ(IS_SORTED(c.begin(), c.end(), lte),   false);
  BOOST_TEST_EQ(IS_SORTED(c.rbegin(), c.rend(), gt),  false);
  BOOST_TEST_EQ(IS_SORTED(c.rbegin(), c.rend(), gte), false);

  return report_errors();
}

