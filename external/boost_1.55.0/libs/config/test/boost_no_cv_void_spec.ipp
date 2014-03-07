//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CV_VOID_SPECIALIZATIONS
//  TITLE:         template specialisations of cv-qualified void
//  DESCRIPTION:   If template specialisations for cv-void types
//                 conflict with a specialisation for void.


namespace boost_no_cv_void_specializations{

template <class T>
struct is_void
{
};

template <>
struct is_void<void>
{};

template <>
struct is_void<const void>
{};

template <>
struct is_void<volatile void>
{};

template <>
struct is_void<const volatile void>
{};

int test()
{
   return 0;
}

}



