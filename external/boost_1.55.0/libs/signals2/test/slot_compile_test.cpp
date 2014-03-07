// Signals2 library
// test for compilation of boost/signals2/slot.hpp

// Copyright Frank Mori Hess 2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signals2/slot.hpp>

void myslot()
{}

int myslot2(int)
{
  return 0;
}

int test_main(int, char*[])
{
  boost::signals2::slot<void (void)> sl0(&myslot);
  return 0;
}
