//  (C) Copyright Steve Cleary & John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <boost/static_assert.hpp>

//
// all these tests should fail:
//
typedef char a1[2];
typedef char a2[3];

// Function (block) scope
void f()
{
  BOOST_STATIC_ASSERT(sizeof(a1) == sizeof(a2)); // should not compile
}





