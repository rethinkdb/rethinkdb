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
#include <iostream>
// What's the proper BOOST_ flag for <iomanip.h> vs <ios>
#include <iomanip>

#include <boost/timer.hpp>
#include <boost/algorithm/minmax.hpp>

template <class T1, class T2>
void tie(std::pair<T1, T2> p, T1& min, T2& max)
{
  min = p.first; max = p.second;
}

template <class Value>
struct less_count : std::less<Value> {
  less_count(less_count<Value> const& lc) : _M_counter(lc._M_counter) {}
  less_count(int& counter) : _M_counter(counter) {}
  bool operator()(Value const& a, Value const& b) const {
    ++_M_counter;
    return std::less<Value>::operator()(a,b);
  }
  void reset() {
    _M_counter = 0;
  }
private:
  int& _M_counter;
};

inline int opt_min_count(int n) {
  return (n==0) ? 0 : n-1;
}
inline int opt_minmax_count(int n) {
  if (n < 2) return 0;
  if (n == 2) return 1;
  return (n%2 == 0) ? 3*(n/2)-1 : 3*(n/2)+1;
}
inline int opt_boost_minmax_count(int n) {
  if (n < 2) return 0;
  if (n == 2) return 1;
  return (n%2 == 0) ? 3*(n/2)-2 : 3*(n/2);
}

int repeats = 10;

#define TIMER( n, cmd , cmdname ) \
  t.restart(); \
  for (int i=0; i<repeats; ++i) { cmd ; } \
  std::cout << "    " << std::setprecision(4) \
            << (double)n*repeats/t.elapsed()/1.0E6 \
            << "M items/sec  " << cmdname << "\n"

#define CTIMER( n, cmd , cmdname, count, opt ) \
  t.restart(); lc.reset(); \
  for (int i=0; i<repeats; ++i) { cmd ; } \
  std::cout << "    " << std::setprecision(4) \
            << (double)n*repeats/t.elapsed()/1.0E6 \
            << "M items/sec  " << cmdname \
            << " ("<< (count)/repeats << " vs " << opt << ")\n"

template <class CIterator>
void test_minmax_element(CIterator first, CIterator last, int n, char* name)
{
  typedef typename std::iterator_traits<CIterator>::value_type vtype;
  boost::timer t;

  std::cout << "  ON " << name << " WITH OPERATOR<()\n";
  TIMER( n, std::min_element(first, last),
    "std::min_element" << name << "");
  TIMER( n, std::max_element(first, last),
    "std::max_element" << name << "");
  TIMER( n, boost::first_min_element(first, last),
    "boost::first_min_element" << name << "");
  TIMER( n, boost::last_min_element(first, last),
    "boost::last_min_element" << name << " ");
  TIMER( n, boost::first_max_element(first, last),
    "boost::first_max_element" << name << "");
  TIMER( n, boost::last_max_element(first, last),
    "boost::last_max_element" << name << " ");
  TIMER( n, boost::minmax_element(first, last),
    "boost::minmax_element" << name << "    ");
  TIMER( n, boost::first_min_last_max_element(first, last),
    "boost::first_min_last_max_element" << name << "");
  TIMER( n, boost::last_min_first_max_element(first, last),
    "boost::last_min_first_max_element" << name << "");
  TIMER( n, boost::last_min_last_max_element(first, last),
    "boost::last_min_last_max_element" << name << " ");

  #define pred std::bind2nd( std::greater<vtype>(), vtype(10) )
  TIMER( n, boost::min_element_if(first, last, pred),
    "boost::min_element_if" << name << "");
  TIMER( n, boost::max_element_if(first, last, pred),
    "boost::max_element_if" << name << "");
  TIMER( n, std::min_element(boost::make_filter_iterator(first, last, pred),
                             boost::make_filter_iterator(last, last, pred)),
    "std::min_element_with_filter_iterator" << name << "");
  TIMER( n, std::max_element(boost::make_filter_iterator(first, last, pred),
                             boost::make_filter_iterator(last, last, pred)),
    "std::max_element_if_with_filter_iterator" << name << "");
  #undef pred

  int counter = 0;
  less_count<vtype> lc(counter);
  std::cout << "  ON " << name << " WITH LESS<> AND COUNTING COMPARISONS\n";
  CTIMER( n, std::min_element(first, last, lc),
    "std::min_element" << name << "        ",
    counter, opt_min_count(n) );
  CTIMER( n, std::max_element(first, last, lc),
    "std::max_element" << name << "        ",
    counter, opt_min_count(n) );
  CTIMER( n, boost::first_min_element(first, last, lc),
    "boost::first_min_element" << name << "",
    counter, opt_min_count(n) );
  CTIMER( n, boost::last_min_element(first, last, lc),
    "boost::last_max_element" << name << " ",
    counter, opt_min_count(n) );
  CTIMER( n, boost::first_max_element(first, last, lc),
    "boost::first_min_element" << name << "",
    counter, opt_min_count(n) );
  CTIMER( n, boost::last_max_element(first, last, lc),
    "boost::last_max_element" << name << " ",
    counter, opt_min_count(n) );
  CTIMER( n, boost::minmax_element(first, last, lc),
    "boost::minmax_element" << name << "   ",
    counter, opt_minmax_count(n) );
  CTIMER( n, boost::first_min_last_max_element(first, last, lc),
    "boost::first_min_last_max_element" << name << "",
    counter, opt_boost_minmax_count(n) );
  CTIMER( n, boost::last_min_first_max_element(first, last, lc),
    "boost::last_min_first_max_element" << name << "",
    counter, opt_boost_minmax_count(n) );
  CTIMER( n, boost::last_min_last_max_element(first, last, lc),
    "boost::last_min_last_max_element" << name << " ",
    counter, opt_minmax_count(n) );
}

template <class Container, class Iterator, class Value>
void test_container(Iterator first, Iterator last, int n, char* name)
{
  Container c(first, last);
  typename Container::iterator fit(c.begin()), lit(c.end());
  test_minmax_element(fit, lit, n, name);
}

template <class Iterator>
void test_range(Iterator first, Iterator last, int n)
{
  typedef typename std::iterator_traits<Iterator>::value_type Value;
  // Test various containers with these values
  test_container< std::vector<Value>, Iterator, Value >(first, last, n, "<vector>");
#ifndef ONLY_VECTOR
  test_container< std::list<Value>,   Iterator, Value >(first, last, n, "<list>  ");
  test_container< std::multiset<Value>,    Iterator, Value >(first, last, n, "<set>   ");
#endif
}

template <class Value>
void test(int n)
{
  // Populate test vector with identical values
  std::cout << "IDENTICAL VALUES...   \n";
  std::vector<Value> test_vector(n, Value(1));
  typename std::vector<Value>::iterator first( test_vector.begin() );
  typename std::vector<Value>::iterator last( test_vector.end() );
  test_range(first, last, n);

  // Populate test vector with two values
  std::cout << "TWO DISTINCT VALUES...\n";
  typename std::vector<Value>::iterator middle( first + n/2 );
  std::fill(middle, last, Value(2));
  test_range(first, last, n);

  // Populate test vector with increasing values
  std::cout << "INCREASING VALUES...  \n";
  std::fill(first, last, Value(1));
  std::accumulate(first, last, Value(0));
  test_range(first, last, n);

  // Populate test vector with decreasing values
  std::cout << "DECREASING VALUES...  \n";
  std::reverse(first, last);
  test_range(first, last, n);

  // Populate test vector with random values
  std::cout << "RANDOM VALUES...      \n";
  std::random_shuffle(first, last);
  test_range(first, last, n);
}

int
main(char argc, char** argv)
{
  int n = 100;
  if (argc > 1) n = atoi(argv[1]);
  if (argc > 2) repeats = atoi(argv[2]);

  test<int>(n);

  return 0;
}
