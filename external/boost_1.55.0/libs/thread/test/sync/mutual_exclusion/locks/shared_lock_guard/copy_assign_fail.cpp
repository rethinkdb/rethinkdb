//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/shared_lock_guard.hpp>

// template <class Mutex> class shared_lock_guard;

// shared_lock_guard& operator=(shared_lock_guard const&) = delete;

#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::shared_mutex m0;
boost::shared_mutex m1;

int main()
{
  boost::shared_lock_guard<boost::shared_mutex> lk0(m0);
  boost::shared_lock_guard<boost::shared_mutex> lk1(m1);
  lk1 = lk0;

}

#include "../../../../remove_error_code_unused_warning.hpp"

