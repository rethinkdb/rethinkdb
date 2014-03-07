// Boost.Signals library

// Copyright Douglas Gregor 2001-2006. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signal.hpp>
#include <boost/bind.hpp>

struct short_lived : public boost::BOOST_SIGNALS_NAMESPACE::trackable {
  ~short_lived() {}
};

struct swallow {
  template<typename T> int operator()(const T*, int i) { return i; }
};

template<typename T>
struct max_or_default {
  typedef T result_type;

  template<typename InputIterator>
  T operator()(InputIterator first, InputIterator last) const
  {
    if (first == last)
      return T();

    T max = *first++;
    for (; first != last; ++first)
      max = (*first > max)? *first : max;

    return max;
  }
};

int test_main(int, char*[])
{
  typedef boost::signal1<int, int, max_or_default<int> > sig_type;
  sig_type s1;

  // Test auto-disconnection
  BOOST_CHECK(s1(5) == 0);
  {
    short_lived shorty;
    s1.connect(boost::bind<int>(swallow(), &shorty, _1));
    BOOST_CHECK(s1(5) == 5);
  }
  BOOST_CHECK(s1(5) == 0);

  // Test auto-disconnection of slot before signal connection
  {
    short_lived* shorty = new short_lived();

    sig_type::slot_type slot(boost::bind<int>(swallow(), shorty, _1));
    delete shorty;

    BOOST_CHECK(s1(5) == 0);
  }

  return 0;
}
