//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_USING_TEMPLATE
//  TITLE:         using template declarations
//  DESCRIPTION:   The compiler will not accept a using declaration
//                 that imports a class or function template
//                 into a named namespace.  Probably Borland/MSVC6 specific.

template <class T>
int global_foo(T)
{
   return 0;
}

template <class T, class U = void>
struct op
{
   friend op<T,U> operator +(const op&, const op&)
   {
      return op();
   };
};

namespace boost_no_using_template{

using ::global_foo;
using ::op;

int test()
{
   boost_no_using_template::op<int, int> a;
   boost_no_using_template::op<int, int> b;
   a+b;
   return boost_no_using_template::global_foo(0);
}

}





