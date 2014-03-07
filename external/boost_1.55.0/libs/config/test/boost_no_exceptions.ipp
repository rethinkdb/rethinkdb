//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_EXCEPTIONS
//  TITLE:         exception handling support
//  DESCRIPTION:   The compiler in its current translation mode supports
//                 exception handling.


namespace boost_no_exceptions{

void throw_it(int i)
{
   throw i;
}

int test()
{
   try
   {
      throw_it(2);
   }
   catch(int i)
   {
      return (i == 2) ? 0 : -1;
   }
   catch(...)
   {
      return -1;
   }
   return -1;
}

}





