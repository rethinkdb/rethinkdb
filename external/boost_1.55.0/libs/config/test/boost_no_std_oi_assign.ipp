//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_OUTPUT_ITERATOR_ASSIGN
//  TITLE:         That the std output iterators are assignable
//  DESCRIPTION:   Some std lib output iterators are not assignable
//                 even this is required by the standard.

#include <iterator>
#include <list>
#include <iostream>


namespace boost_no_std_output_iterator_assign {

int test()
{
   std::list<int> l;
   std::back_insert_iterator<std::list<int> > bi1(l);
   std::back_insert_iterator<std::list<int> > bi2(l);
   bi1 = bi2;

   std::front_insert_iterator<std::list<int> > fi1(l);
   std::front_insert_iterator<std::list<int> > fi2(l);
   fi1 = fi2;

   std::ostream_iterator<char> osi1(std::cout);
   std::ostream_iterator<char> osi2(std::cout);
   osi1 = osi2;
   
   return 0;
}

}




