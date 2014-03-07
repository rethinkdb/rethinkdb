//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_EXCEPTION_STD_NAMESPACE
//  TITLE:         <exception> is in namespace std
//  DESCRIPTION:   Some std libs don't put the contents of
//                 <exception> in namespace std.

#include <exception>

namespace boost_no_exception_std_namespace{

int t(int i)
{
   if(i)
      std::terminate();
   return 0;
}

int test()
{
   return t(0);
}

}






