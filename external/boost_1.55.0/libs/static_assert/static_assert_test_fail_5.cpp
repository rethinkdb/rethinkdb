//  (C) Copyright Steve Cleary & John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <boost/static_assert.hpp>

//
// all these tests should fail:
//

// Template class scope
template <class Int, class Char>
struct Bill
{
  private:  // can be in private, to avoid namespace pollution
    BOOST_STATIC_ASSERT(sizeof(Int) == 4);
    BOOST_STATIC_ASSERT(sizeof(Int) == sizeof(Char)); // should not compile when instantiated
  public:

  // Template member function scope: provides access to member variables
  Int x;
  Char c;
  template <class Int2, class Char2>
  void f(Int2 , Char2 )
  {
    BOOST_STATIC_ASSERT(sizeof(Int) == sizeof(Int2));
    BOOST_STATIC_ASSERT(sizeof(Char) == sizeof(Char2));
    //BOOST_STATIC_ASSERT(sizeof(Int) == sizeof(Char)); // should not compile when instantiated
  }
};

Bill<int, char> b;




