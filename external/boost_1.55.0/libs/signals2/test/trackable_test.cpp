// Boost.Signals2 library

// Copyright Douglas Gregor 2001-2006.
// Copyright Frank Mori Hess 2009.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/signals2/trackable.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/weak_ptr.hpp>

struct short_lived : public boost::signals2::trackable {
  ~short_lived() {}
};

struct swallow {
  typedef int result_type;
  template<typename T> int operator()(const T*, int i) { return i; }
  template<typename T> int operator()(T &, int i) { return i; }
  template<typename T> int operator()(boost::weak_ptr<T>, int i) { return i; }
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
  typedef boost::signals2::signal<int (int), max_or_default<int> > sig_type;
  sig_type s1;

  // Test auto-disconnection
  BOOST_CHECK(s1(5) == 0);
  {
    short_lived shorty;
    s1.connect(boost::bind<int>(swallow(), &shorty, _1));
    BOOST_CHECK(s1(5) == 5);
  }
  BOOST_CHECK(s1(5) == 0);
  // Test auto-disconnection of trackable inside reference_wrapper
  {
    short_lived shorty;
    s1.connect(boost::bind<int>(swallow(), boost::ref(shorty), _1));
    BOOST_CHECK(s1(5) == 5);
  }
  BOOST_CHECK(s1(5) == 0);

  // Test multiple arg slot constructor
  {
    short_lived shorty;
    s1.connect(sig_type::slot_type(swallow(), &shorty, _1));
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
