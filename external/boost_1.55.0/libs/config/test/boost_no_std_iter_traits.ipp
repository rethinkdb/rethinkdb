//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

//  MACRO:         BOOST_NO_STD_ITERATOR_TRAITS
//  TITLE:         std::iterator_traits
//  DESCRIPTION:   The compiler does not provide a standard
//                 compliant implementation of std::iterator_traits.
//                 Note that the compiler may still have a non-standard
//                 implementation.

#include <iterator>
#include <stddef.h>

namespace boost_no_std_iterator_traits{

struct UDT_iterator
{
   typedef int value_type;
   typedef ptrdiff_t difference_type;
   typedef int* pointer;
   typedef int& reference;
   typedef std::input_iterator_tag iterator_category;
};

struct UDT{};


int test()
{
   std::iterator_traits<UDT_iterator>::value_type v = 0;
   std::iterator_traits<UDT_iterator>::difference_type d = 0;
   std::iterator_traits<UDT_iterator>::pointer p = &v;
   std::iterator_traits<UDT_iterator>::reference r = v;
   std::iterator_traits<UDT_iterator>::iterator_category cat;

   std::iterator_traits<UDT*>::value_type v2;
   std::iterator_traits<UDT*>::difference_type d2 = 0;
   std::iterator_traits<UDT*>::pointer p2 = &v2;
   std::iterator_traits<UDT*>::reference r2 = v2;
   std::iterator_traits<UDT*>::iterator_category cat2;

   std::iterator_traits<const UDT*>::value_type v3;
   std::iterator_traits<const UDT*>::difference_type d3 = 0;
   std::iterator_traits<const UDT*>::pointer p3 = &v3;
   std::iterator_traits<const UDT*>::reference r3 = v3;
   std::iterator_traits<const UDT*>::iterator_category cat3;

   //
   // suppress some warnings:
   //
   (void) &v;
   (void) &d;
   (void) &p;
   (void) &r;
   (void) &cat;

   (void) &v2;
   (void) &d2;
   (void) &p2;
   (void) &r2;
   (void) &cat2;

   (void) &v3;
   (void) &d3;
   (void) &p3;
   (void) &r3;
   (void) &cat3;

   return 0;
}

}




