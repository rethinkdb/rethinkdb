// Boost.Signals library

// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signal.hpp>
#include <functional>

template<typename T>
struct max_or_default {
  typedef T result_type;
  template<typename InputIterator>
  typename InputIterator::value_type
  operator()(InputIterator first, InputIterator last) const
  {
    if (first == last)
      return T();

    T max = *first++;
    for (; first != last; ++first)
      max = (*first > max)? *first : max;

    return max;
  }
};

struct make_int {
  make_int(int n, int cn) : N(n), CN(n) {}
  int operator()() { return N; }
  int operator()() const { return CN; }

  int N;
  int CN;
};

template<int N>
struct make_increasing_int {
  make_increasing_int() : n(N) {}

  int operator()() const { return n++; }

  mutable int n;
};

int get_37() { return 37; }

static void
test_zero_args()
{
  make_int i42(42, 41);
  make_int i2(2, 1);
  make_int i72(72, 71);
  make_int i63(63, 63);
  make_int i62(62, 61);

  {
    boost::signal0<int, max_or_default<int>, std::string> s0;
    boost::BOOST_SIGNALS_NAMESPACE::connection c2 = s0.connect(i2);
    boost::BOOST_SIGNALS_NAMESPACE::connection c72 = s0.connect("72", i72);
    boost::BOOST_SIGNALS_NAMESPACE::connection c62 = s0.connect("6x", i62);
    boost::BOOST_SIGNALS_NAMESPACE::connection c42 = s0.connect(i42);
    boost::BOOST_SIGNALS_NAMESPACE::connection c37 = s0.connect(&get_37);

    BOOST_CHECK(s0() == 72);

    s0.disconnect("72");
    BOOST_CHECK(s0() == 62);

    c72.disconnect(); // Double-disconnect should be safe
    BOOST_CHECK(s0() == 62);

    s0.disconnect("72"); // Triple-disconect should be safe
    BOOST_CHECK(s0() == 62);

    // Also connect 63 in the same group as 62
    s0.connect("6x", i63);
    BOOST_CHECK(s0() == 63);

    // Disconnect all of the 60's
    s0.disconnect("6x");
    BOOST_CHECK(s0() == 42);

    c42.disconnect();
    BOOST_CHECK(s0() == 37);

    c37.disconnect();
    BOOST_CHECK(s0() == 2);

    c2.disconnect();
    BOOST_CHECK(s0() == 0);
  }

  {
    boost::signal0<int, max_or_default<int> > s0;
    boost::BOOST_SIGNALS_NAMESPACE::connection c2 = s0.connect(i2);
    boost::BOOST_SIGNALS_NAMESPACE::connection c72 = s0.connect(i72);
    boost::BOOST_SIGNALS_NAMESPACE::connection c62 = s0.connect(i62);
    boost::BOOST_SIGNALS_NAMESPACE::connection c42 = s0.connect(i42);

    const boost::signal0<int, max_or_default<int> >& cs0 = s0;
    BOOST_CHECK(cs0() == 72);
  }

  {
    make_increasing_int<7> i7;
    make_increasing_int<10> i10;

    boost::signal0<int, max_or_default<int> > s0;
    boost::BOOST_SIGNALS_NAMESPACE::connection c7 = s0.connect(i7);
    boost::BOOST_SIGNALS_NAMESPACE::connection c10 = s0.connect(i10);

    BOOST_CHECK(s0() == 10);
    BOOST_CHECK(s0() == 11);
  }
}

static void
test_one_arg()
{
  boost::signal1<int, int, max_or_default<int> > s1;

  s1.connect(std::negate<int>());
  s1.connect(std::bind1st(std::multiplies<int>(), 2));

  BOOST_CHECK(s1(1) == 2);
  BOOST_CHECK(s1(-1) == 1);
}

static void
test_signal_signal_connect()
{
  boost::signal1<int, int, max_or_default<int> > s1;

  s1.connect(std::negate<int>());

  BOOST_CHECK(s1(3) == -3);

  {
    boost::signal1<int, int, max_or_default<int> > s2;
    s1.connect(s2);
    s2.connect(std::bind1st(std::multiplies<int>(), 2));
    s2.connect(std::bind1st(std::multiplies<int>(), -3));

    BOOST_CHECK(s2(-3) == 9);
    BOOST_CHECK(s1(3) == 6);
  } // s2 goes out of scope and disconnects

  BOOST_CHECK(s1(3) == -3);
}

struct EventCounter
{
  EventCounter() : count(0) {}

  void operator()()
  {
    ++count;
  }

  int count;
};

static void
test_ref()
{
  EventCounter ec;
  boost::signal0<void> s;

  {
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection c = s.connect(boost::ref(ec));
    BOOST_CHECK(ec.count == 0);
    s();
    BOOST_CHECK(ec.count == 1);
  }
  s();
  BOOST_CHECK(ec.count == 1);
}

int
test_main(int, char* [])
{
  test_zero_args();
  test_one_arg();
  test_signal_signal_connect();
  test_ref();
  return 0;
}
