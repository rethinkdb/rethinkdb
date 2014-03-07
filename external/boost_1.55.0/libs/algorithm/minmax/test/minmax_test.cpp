//  (C) Copyright Herve Bronnimann 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <functional>

#include <boost/config.hpp>
#include <boost/algorithm/minmax.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

class custom {
  int m_x;
  friend std::ostream& operator<<(std::ostream& str, custom const& x);
public:
  explicit custom(int x = 0) : m_x(x) {}
  custom(custom const& y) : m_x(y.m_x) {}
  bool operator==(custom const& y) const { return m_x == y.m_x; }
  bool operator<(custom const& y) const { return m_x < y.m_x; }
  custom operator+(custom const& y) const { return custom(m_x+y.m_x); }
  custom& operator+=(custom const& y) { m_x += y.m_x; return *this; }
};

std::ostream&
operator<<(std::ostream& str, custom const& x)
{
  return  str << x.m_x;
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

using namespace boost;

template <class Value>
void test(BOOST_EXPLICIT_TEMPLATE_TYPE(Value))
{
  Value zero(0), one(1);
  int counter = 0;
  less_count<Value> lc(counter);

  // Test functionality
  tuple<Value const&, Value const&> result1 = boost::minmax(zero, one);
  BOOST_CHECK_EQUAL( get<0>(result1), zero );
  BOOST_CHECK_EQUAL( get<1>(result1), one );

  tuple<Value const&, Value const&> result2 = boost::minmax(one, zero);
  BOOST_CHECK_EQUAL( get<0>(result2), zero );
  BOOST_CHECK_EQUAL( get<1>(result2), one );
  
  // Test functionality and number of comparisons
  lc.reset();
  tuple<Value const&, Value const&> result3 = boost::minmax(zero, one, lc );
  BOOST_CHECK_EQUAL( get<0>(result3), zero );
  BOOST_CHECK_EQUAL( get<1>(result3), one );
  BOOST_CHECK_EQUAL( counter, 1 );

  lc.reset();
  tuple<Value const&, Value const&> result4 = boost::minmax(one, zero, lc );
  BOOST_CHECK_EQUAL( get<0>(result4), zero );
  BOOST_CHECK_EQUAL( get<1>(result4), one );
  BOOST_CHECK_EQUAL( counter, 1);
}

BOOST_AUTO_TEST_CASE( test_main )
{
  test<int>(); // ("builtin");
  test<custom>(); // ("custom ");
}
