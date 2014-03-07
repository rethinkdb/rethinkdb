// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class strict_lock;

// strict_lock(strict_lock const&) = delete;


#include <boost/thread/strict_lock.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::mutex m0;
boost::mutex m1;

int main()
{
  boost::strict_lock<boost::mutex> lk0(m0);
  boost::strict_lock<boost::mutex> lk1 = lk0;
}

#include "../../../../remove_error_code_unused_warning.hpp"

