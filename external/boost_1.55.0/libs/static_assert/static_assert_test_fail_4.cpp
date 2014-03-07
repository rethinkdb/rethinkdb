//  (C) Copyright Steve Cleary & John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <boost/config.hpp>
#include <boost/static_assert.hpp>

//
// all these tests should fail:
//


struct Bob
{
  public:

  // Member function scope: provides access to member variables
  char x[4];
  char c;
  int f()
  {
#if !defined(BOOST_MSVC) || BOOST_MSVC > 1200 // broken sizeof in VC6
    BOOST_STATIC_ASSERT(sizeof(x) == 4);
    BOOST_STATIC_ASSERT(sizeof(c) == 1);
    BOOST_STATIC_ASSERT((sizeof(x) == sizeof(c))); // should not compile
#endif
    return x;
  }
};






