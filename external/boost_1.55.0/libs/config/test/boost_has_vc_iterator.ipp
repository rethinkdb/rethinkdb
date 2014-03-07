//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_MSVC_STD_ITERATOR
//  TITLE:         microsoft's version of std::iterator
//  DESCRIPTION:   Microsoft's broken version of std::iterator
//                 is being used.

#include <iterator>

namespace boost_msvc_std_iterator{

int test()
{
   typedef std::iterator<
      std::random_access_iterator_tag,
      int
      > iterator_type_2;
   typedef std::reverse_iterator<const char*, const char> r_it;

   iterator_type_2::value_type v2 = 0;
   iterator_type_2::iterator_category cat2;
   //
   // suppress some warnings:
   //
   (void)v2;
   (void)cat2;

   return 0;
}

}




