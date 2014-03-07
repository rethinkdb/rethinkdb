//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_TEMPLATED_IOSTREAMS
//  TITLE:         basic_iostream<>
//  DESCRIPTION:   The platform supports "new style" templated iostreams.

#include <iostream>


namespace boost_no_templated_iostreams{

int test()
{
   std::basic_ostream<char, std::char_traits<char> >& osr = std::cout;
   (void)osr;
   return 0;
}

}





