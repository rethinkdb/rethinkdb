//  (C) Copyright Herve Bronnimann 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <functional>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <vector>
#include <list>
#include <set>
#include <cstdlib>

#include <boost/config.hpp> /* prevents some nasty warns in MSVC */
#include <boost/algorithm/minmax_element.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

class custom {
  int m_x;
  friend bool operator<(custom const& x, custom const& y);
public:
  explicit custom(int x = 0) : m_x(x) {}
  custom(custom const& y) : m_x(y.m_x) {}
  custom operator+(custom const& y) const { return custom(m_x+y.m_x); }
  custom& operator+=(custom const& y) { m_x += y.m_x; return *this; }
};

bool operator< (custom const& x, custom const& y)
{
  return x.m_x < y.m_x;
}

BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(custom)

namespace std {

template <>
struct iterator_traits<int*> {
  typedef random_access_iterator_tag  iterator_category;
  typedef int                          value_type;
  typedef ptrdiff_t                    difference_type;
  typedef value_type*                  pointer;
  typedef value_type&                  reference;
};

template <>
struct iterator_traits<custom*> {
  typedef random_access_iterator_tag  iterator_category;
  typedef custom                       value_type;
  typedef ptrdiff_t                    difference_type;
  typedef value_type*                  pointer;
  typedef value_type&                  reference;
};

}

template <class T1, class T2, class T3, class T4>
void tie(std::pair<T1, T2> p, T3& first, T4& second)
{
  first = T3(p.first); second = T4(p.second);
}

template <class Value>
struct less_count : std::less<Value> {
    typedef std::less<Value> Base;
  less_count(less_count<Value> const& lc) : m_counter(lc.m_counter) {}
  less_count(int& counter) : m_counter(counter) {}
  bool operator()(Value const& a, Value const& b) const {
    ++m_counter;
    return Base::operator()(a,b);
  }
  void reset() {
    m_counter = 0;
  }
private:
  int& m_counter;
};

inline int opt_min_count(int n) {
  return (n==0) ? 0 : n-1;
}
inline int opt_minmax_count(int n) {
  if (n < 2) return 0;
  if (n == 2) return 2;
  return (n%2 == 0) ? 3*(n/2)-1 : 3*(n/2)+1;
}
inline int opt_boost_minmax_count(int n) {
  if (n < 2) return 0;
  if (n == 2) return 1;
  return (n%2 == 0) ? 3*(n/2)-2 : 3*(n/2);
}

#define CHECK_EQUAL_ITERATORS( left, right, first ) \
BOOST_CHECK_EQUAL( std::distance( first, left ), std::distance( first, right ) )

template <class CIterator>
void test_minmax(CIterator first, CIterator last, int n)
{
  using namespace boost;

  typedef typename std::iterator_traits<CIterator>::value_type Value;
  typedef boost::reverse_iterator<CIterator> RCIterator;
  // assume that CIterator is BidirectionalIter
  CIterator min, max;
  RCIterator rfirst(last), rlast(first), rmin, rmax;
  int counter = 0;
  less_count<Value> lc(counter);

  // standard extensions
  // first version, operator<
  tie( boost::minmax_element(first, last), min, max );

  CHECK_EQUAL_ITERATORS( min, std::min_element(first, last), first );
  CHECK_EQUAL_ITERATORS( max, std::max_element(first, last), first );
  
  // second version, comp function object (keeps a counter!)
  lc.reset();
  tie( boost::minmax_element(first, last, lc), min, max );
  BOOST_CHECK( counter <= opt_minmax_count(n) );
  CHECK_EQUAL_ITERATORS( min, std::min_element(first, last, lc), first );
  CHECK_EQUAL_ITERATORS( max, std::max_element(first, last, lc), first );

  // boost extensions
  // first version, operator<
  CHECK_EQUAL_ITERATORS( boost::first_min_element(first, last), std::min_element(first, last), first );
  rmin = RCIterator(boost::last_min_element(first, last));
  rmin = (rmin == rfirst) ? rlast : --rmin;
  CHECK_EQUAL_ITERATORS( rmin, std::min_element(rfirst, rlast), rfirst );
  CHECK_EQUAL_ITERATORS( boost::first_max_element(first, last), std::max_element(first, last), first );
  rmax = RCIterator(boost::last_max_element(first, last));
  rmax = (rmax == rfirst) ? rlast : --rmax;
  CHECK_EQUAL_ITERATORS( rmax, std::max_element(rfirst, rlast), rfirst );
  tie( boost::first_min_last_max_element(first, last), min, max );
  CHECK_EQUAL_ITERATORS( min, boost::first_min_element(first, last), first );
  CHECK_EQUAL_ITERATORS( max, boost::last_max_element(first, last), first );
  tie( boost::last_min_first_max_element(first, last), min, max );
  CHECK_EQUAL_ITERATORS( min, boost::last_min_element(first, last), first );
  CHECK_EQUAL_ITERATORS( max, boost::first_max_element(first, last), first );
  tie( boost::last_min_last_max_element(first, last), min, max );
  CHECK_EQUAL_ITERATORS( min, boost::last_min_element(first, last), first );
  CHECK_EQUAL_ITERATORS( max, boost::last_max_element(first, last), first );

  // second version, comp function object (keeps a counter!)
  lc.reset();
  min = boost::first_min_element(first, last, lc);
  BOOST_CHECK( counter <= opt_min_count(n) );
  CHECK_EQUAL_ITERATORS( min, std::min_element(first, last, lc), first );
  lc.reset();
  rmin = RCIterator(boost::last_min_element(first, last, lc));
  rmin = (rmin == rfirst) ? rlast : --rmin;
  BOOST_CHECK( counter <= opt_min_count(n) );
  CHECK_EQUAL_ITERATORS( rmin, std::min_element(rfirst, rlast, lc), rfirst );
  lc.reset();
  max =  boost::first_max_element(first, last, lc);
  BOOST_CHECK( counter <= opt_min_count(n) );
  CHECK_EQUAL_ITERATORS( max, std::max_element(first, last, lc), first );
  lc.reset();
  rmax = RCIterator(boost::last_max_element(first, last, lc));
  rmax = (rmax == rfirst) ? rlast : --rmax;
  BOOST_CHECK( counter <= opt_min_count(n) );
  CHECK_EQUAL_ITERATORS( rmax, std::max_element(rfirst, rlast, lc), rfirst );
  lc.reset();
  tie( boost::first_min_last_max_element(first, last, lc), min, max );
  BOOST_CHECK( counter <= opt_boost_minmax_count(n) );
  CHECK_EQUAL_ITERATORS( min, boost::first_min_element(first, last, lc), first );
  CHECK_EQUAL_ITERATORS( max, boost::last_max_element(first, last, lc), first );
  lc.reset();
  tie( boost::last_min_first_max_element(first, last, lc), min, max );
  BOOST_CHECK( counter <= opt_boost_minmax_count(n) );
  BOOST_CHECK( min == boost::last_min_element(first, last, lc) );
  CHECK_EQUAL_ITERATORS( max, boost::first_max_element(first, last, lc), first );
  lc.reset();
  tie( boost::last_min_last_max_element(first, last, lc), min, max );
  BOOST_CHECK( counter <= opt_minmax_count(n) );
  CHECK_EQUAL_ITERATORS( min, boost::last_min_element(first, last, lc), first );
  CHECK_EQUAL_ITERATORS( max, boost::last_max_element(first, last, lc), first );
}

template <class Container, class Iterator, class Value>
void test_container(Iterator first, Iterator last, int n,
                    Container* dummy = 0
                    BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Value) )
{
  Container c(first, last);
  test_minmax(c.begin(), c.end(), n);
}

template <class Iterator>
void test_range(Iterator first, Iterator last, int n)
{
  typedef typename std::iterator_traits<Iterator>::value_type Value;
  // Test various containers with these values
  test_container< std::vector<Value>, Iterator, Value >(first, last, n);
  test_container< std::list<Value>,   Iterator, Value >(first, last, n);
  test_container< std::set<Value>,    Iterator, Value >(first, last, n);
}

template <class Value>
void test(int n BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Value))
{
  // Populate test vector with identical values
  std::vector<Value> test_vector(n, Value(1));
  typename std::vector<Value>::iterator first( test_vector.begin() );
  typename std::vector<Value>::iterator last( test_vector.end() );
  test_range(first, last, n);

  // Populate test vector with two values
  typename std::vector<Value>::iterator middle( first + n/2 );
  std::fill(middle, last, Value(2));
  test_range(first, last, n);

  // Populate test vector with increasing values
  std::accumulate(first, last, Value(0));
  test_range(first, last, n);

  // Populate test vector with decreasing values
  std::reverse(first, last);
  test_range(first, last, n);

  // Populate test vector with random values
  std::random_shuffle(first, last);
  test_range(first, last, n);
}

BOOST_AUTO_TEST_CASE( test_main )
{
#ifndef BOOST_NO_STDC_NAMESPACE
  using std::atoi;
#endif

  int n = 100;

  test<int>(n);
  test<custom>(n);
}
