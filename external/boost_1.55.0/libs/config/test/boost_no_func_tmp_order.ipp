//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_FUNCTION_TEMPLATE_ORDERING
//  TITLE:         no function template ordering
//  DESCRIPTION:   The compiler does not perform 
//                 function template ordering or its function
//                 template ordering is incorrect.
//  
//                 template<typename T> void f(T); // #1
//                 template<typename T, typename U> void f(T (*)(U)); // #2
//                 void bar(int);
//                 f(&bar); // should choose #2.


namespace boost_no_function_template_ordering{

template<typename T>
bool f(T)
{
   return false;
}

template<typename T, typename U>
bool f(T (*)(U))
{
   return true;
}

void bar(int)
{
}

int test()
{
   int i = 0;
   return f(i) || !f(&bar);
}

}





