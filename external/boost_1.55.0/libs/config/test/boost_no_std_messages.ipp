//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_MESSAGES
//  TITLE:         std::messages<charT>
//  DESCRIPTION:   The standard library lacks a conforming std::messages facet.

#include <locale>


namespace boost_no_std_messages{

//
// this just has to complile, not run:
//
void test_messages(const std::messages<char>& m)
{
   static const std::locale l;
   static const std::string name("foobar");
   m.close(m.open(name, l));
}

int test()
{
   const std::messages<char>* pmf = 0;
   (void) &pmf;
   return 0;
}

}





