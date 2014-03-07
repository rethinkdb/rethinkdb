//  (C) Copyright Steve Cleary & John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <boost/static_assert.hpp>

//
// all these tests should succeed.
// some of these tests are rather simplistic (ie useless)
// in order to ensure that they compile on all platforms.
//

// Namespace scope
BOOST_STATIC_ASSERT(sizeof(int) >= sizeof(short));
BOOST_STATIC_ASSERT(sizeof(char) == 1);
BOOST_STATIC_ASSERT_MSG(sizeof(int) >= sizeof(short), "msg1");
BOOST_STATIC_ASSERT_MSG(sizeof(char) == 1, "msg2");

// Function (block) scope
void f()
{
  BOOST_STATIC_ASSERT(sizeof(int) >= sizeof(short));
  BOOST_STATIC_ASSERT(sizeof(char) == 1);
  BOOST_STATIC_ASSERT_MSG(sizeof(int) >= sizeof(short), "msg3");
  BOOST_STATIC_ASSERT_MSG(sizeof(char) == 1, "msg4");
}

struct Bob
{
  private:  // can be in private, to avoid namespace pollution
    BOOST_STATIC_ASSERT(sizeof(int) >= sizeof(short));
    BOOST_STATIC_ASSERT(sizeof(char) == 1);
    BOOST_STATIC_ASSERT_MSG(sizeof(int) >= sizeof(short), "msg5");
    BOOST_STATIC_ASSERT_MSG(sizeof(char) == 1, "msg6");
  public:

  // Member function scope: provides access to member variables
  int x;
  char c;
  int f()
  {
#if defined(_MSC_VER) && _MSC_VER < 1300 // broken sizeof in VC6
    BOOST_STATIC_ASSERT(sizeof(x) >= sizeof(short));
    BOOST_STATIC_ASSERT(sizeof(c) == 1);
    BOOST_STATIC_ASSERT_MSG(sizeof(x) >= sizeof(short), "msg7");
    BOOST_STATIC_ASSERT_MSG(sizeof(c) == 1, "msg8");
#endif
    return x;
  }
};



// Template class scope
template <class Int, class Char>
struct Bill
{
   BOOST_STATIC_CONSTANT(int, value = 1);
  private:  // can be in private, to avoid namespace pollution
    BOOST_STATIC_ASSERT(sizeof(Int) > sizeof(char));
    BOOST_STATIC_ASSERT_MSG(sizeof(Int) > sizeof(char), "msg9");
  public:

  // Template member function scope: provides access to member variables
  Int x;
  Char c;
  template <class Int2, class Char2>
  void f(Int2 , Char2 )
  {
    BOOST_STATIC_ASSERT(sizeof(Int) == sizeof(Int2));
    BOOST_STATIC_ASSERT(sizeof(Char) == sizeof(Char2));
    BOOST_STATIC_ASSERT_MSG(sizeof(Int) == sizeof(Int2), "msg10");
    BOOST_STATIC_ASSERT_MSG(sizeof(Char) == sizeof(Char2), "msg11");
  }
};

void test_Bill() // BOOST_STATIC_ASSERTs are not triggerred until instantiated
{
  Bill<int, char> z;
  //Bill<int, int> bad; // will not compile
  int i = 3;
  char ch = 'a';
  z.f(i, ch);
  //z.f(i, i); // should not compile
}

int main()
{ 
   test_Bill();
   //
   // Test variadic macro support:
   //
#ifndef BOOST_NO_CXX11_VARIADIC_MACROS
   BOOST_STATIC_ASSERT(Bill<int, char>::value);
#ifndef BOOST_NO_CXX11_STATIC_ASSERT
   BOOST_STATIC_ASSERT_MSG(Bill<int, char>::value, "This is a message");
#endif
#endif
   return 0; 
}




