// thread_safe_signals library
// basic test for alternate threading models

// Copyright Frank Mori Hess 2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

// note boost/test/minimal.hpp can cause windows.h to get included, which
// can screw up our checks of _WIN32_WINNT if it is included
// after boost/signals2/mutex.hpp.  Frank Hess 2009-03-07.
#include <boost/test/minimal.hpp>

#include <boost/signals2.hpp>
#include <boost/thread/mutex.hpp>

// combiner that returns the number of slots invoked
struct slot_counter {
  typedef unsigned result_type;
  template<typename InputIterator>
  unsigned operator()(InputIterator first, InputIterator last) const
  {
    unsigned count = 0;
    for (; first != last; ++first)
    {
      try
      {
        *first;
        ++count;
      }
      catch(const boost::bad_weak_ptr &)
      {}
    }
    return count;
  }
};

void myslot()
{
}

template<typename signal_type>
void simple_test()
{
  signal_type sig;
  sig.connect(typename signal_type::slot_type(&myslot));
  BOOST_CHECK(sig() == 1);
  sig.disconnect(&myslot);
  BOOST_CHECK(sig() == 0);
}

int test_main(int, char*[])
{
  typedef boost::signals2::signal<void (), slot_counter, int, std::less<int>, boost::function<void ()>,
    boost::function<void (const boost::signals2::connection &)>, boost::mutex> sig0_mt_type;
  simple_test<sig0_mt_type>();
  typedef boost::signals2::signal<void (), slot_counter, int, std::less<int>, boost::function<void ()>,
    boost::function<void (const boost::signals2::connection &)>, boost::signals2::dummy_mutex> sig0_st_type;
  simple_test<sig0_st_type>();
  return 0;
}
