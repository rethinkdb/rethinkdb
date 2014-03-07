//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CV_SPECIALIZATIONS
//  TITLE:         template specialisations of cv-qualified types
//  DESCRIPTION:   If template specialisations for cv-qualified types
//                 conflict with a specialisation for a cv-unqualififed type.


namespace boost_no_cv_specializations{

template <class T>
struct is_int
{
};

template <>
struct is_int<int>
{};

template <>
struct is_int<const int>
{};

template <>
struct is_int<volatile int>
{};

template <>
struct is_int<const volatile int>
{};

int test()
{
   return 0;
}


}




