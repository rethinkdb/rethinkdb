//  (C) Copyright Steve Cleary & John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <boost/static_assert.hpp>

//
// this tests should fail:
//
typedef char a1[2];
typedef char a2[3];

struct Bob
{
  private:  // can be in private, to avoid namespace pollution
    BOOST_STATIC_ASSERT(sizeof(a1) == sizeof(a2)); // will not compile
  public:

  // Member function scope: provides access to member variables
  int x;
  char c;
  int f()
  {
#ifndef _MSC_VER // broken sizeof in VC6
    BOOST_STATIC_ASSERT(sizeof(x) == 4);
    BOOST_STATIC_ASSERT(sizeof(c) == 1);
#endif
    return x;
  }
};






