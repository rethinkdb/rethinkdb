//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/shared_lock_guard.hpp>

// <mutex>

// template <class Mutex>
// class shared_lock_guard
// {
// public:
//     typedef Mutex mutex_type;
//     ...
// };


#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  BOOST_STATIC_ASSERT_MSG((boost::is_same<boost::shared_lock_guard<boost::shared_mutex>::mutex_type,
      boost::shared_mutex>::value), "");

  return boost::report_errors();
}

