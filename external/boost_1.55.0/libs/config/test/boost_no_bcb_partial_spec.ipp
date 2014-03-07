//  (C) Copyright Terje Slettebo 2002. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//  MACRO:         BOOST_BCB_PARTIAL_SPECIALIZATION_BUG
//  TITLE:         Full partial specialization support.
//  DESCRIPTION:   Borland C++ Builder has some rather specific partial
//                 specialisation bugs which this code tests for.


#include <vector>

namespace boost_bcb_partial_specialization_bug{


template<class T1,class T2>
class Test
{
};

template<class T1,class T2>
class Test<std::vector<T1>,T2>
{
};

template<class T>
class Test<std::vector<int>,T>
{
};

template <class T>
struct is_const{};
template <class T>
struct is_const<T const>{};

int test()
{
   Test<std::vector<int>,double> v;
   is_const<const int> ci;
   (void)v; // warning suppression
   (void)ci;
   return 0;
}


}





