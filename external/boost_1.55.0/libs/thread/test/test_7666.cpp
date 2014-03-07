// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_CHRONO_VERSION 2
#define BOOST_THREAD_VERSION 2

#include <boost/thread/thread_only.hpp>

void myFunc()
{
  boost::this_thread::sleep(boost::posix_time::seconds(5));
}

int main(int, char **)
{
  boost::thread p(myFunc);

  p.join();

  return 0;
}
