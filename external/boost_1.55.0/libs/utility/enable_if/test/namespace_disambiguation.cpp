// Boost enable_if library

// Copyright 2003 (c) The Trustees of Indiana University.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//    Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
//             Jeremiah Willcock (jewillco at osl.iu.edu)
//             Andrew Lumsdaine (lums at osl.iu.edu)

#include <boost/test/minimal.hpp>
#include <boost/mpl/not.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

using boost::enable_if;
using boost::mpl::not_;
using boost::is_arithmetic;

namespace A {
  template<class T>
  typename enable_if<is_arithmetic<T>, bool>::type
  arithmetic_object(T t) { return true; }
}

namespace B {
  template<class T>
  typename enable_if<not_<is_arithmetic<T> >, bool>::type
  arithmetic_object(T t) { return false; }
}

int test_main(int, char*[])
{
  using namespace A;
  using namespace B;
  BOOST_CHECK(arithmetic_object(1));
  BOOST_CHECK(arithmetic_object(1.0));

  BOOST_CHECK(!arithmetic_object("1"));  
  BOOST_CHECK(!arithmetic_object(static_cast<void*>(0)));  

  return 0;
}

