// Boost enable_if library

// Copyright 2003 (c) The Trustees of Indiana University.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//    Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
//             Jeremiah Willcock (jewillco at osl.iu.edu)
//             Andrew Lumsdaine (lums at osl.iu.edu)

#include <boost/test/minimal.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

using boost::enable_if;
using boost::disable_if;
using boost::is_arithmetic;

struct container {
  template <class T>
  typename enable_if<is_arithmetic<T>, bool>::type
  arithmetic_object(const T&, const int* /* disambiguate */ = 0) {return true;}

  template <class T>
  typename disable_if<is_arithmetic<T>, bool>::type
  arithmetic_object(const T&) {return false;}
};

int test_main(int, char*[])
{
 
  BOOST_CHECK(container().arithmetic_object(1));
  BOOST_CHECK(container().arithmetic_object(1.0));

  BOOST_CHECK(!container().arithmetic_object("1"));  
  BOOST_CHECK(!container().arithmetic_object(static_cast<void*>(0)));  

  return 0;
}

