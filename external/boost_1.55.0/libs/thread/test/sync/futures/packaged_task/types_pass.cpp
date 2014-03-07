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

// <boost/thread/future.hpp>

// template<class R, class... ArgTypes>
//     class packaged_task<R(ArgTypes...)>
// {
// public:
//     typedef R result_type;


#include <boost/thread/future.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/lightweight_test.hpp>

struct A {};

int main()
{
  //BOOST_STATIC_ASSERT_MSG((boost::is_same<boost::packaged_task<A>::result_type, A>::value), "");

  return boost::report_errors();
}

