// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/thread_only.hpp>
#include <boost/thread/xtime.hpp>

int main()
{
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.sec += 1;
    boost::thread::sleep(xt); // Sleep for 1 second
}
