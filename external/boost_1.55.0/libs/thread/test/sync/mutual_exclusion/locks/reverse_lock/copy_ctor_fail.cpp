// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class reverse_lock;

// reverse_lock(reverse_lock const&) = delete;


#include <boost/thread/lock_types.hpp>
#include <boost/thread/reverse_lock.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>

boost::mutex m0;
boost::mutex m1;

int main()
{
  boost::mutex m0;
  boost::unique_lock<boost::mutex> lk0(m0);
  {
    boost::reverse_lock<boost::unique_lock<boost::mutex> > lg0(lk0);
    boost::reverse_lock<boost::unique_lock<boost::mutex> > lg1(lg0);
  }
}

#include "../../../../remove_error_code_unused_warning.hpp"

