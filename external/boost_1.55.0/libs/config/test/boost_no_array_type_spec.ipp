//  (C) Copyright John Maddock 2001.
//  (C) Copyright Aleksey Gurtovoy 2003.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS
//  TITLE:         template specialisations of array types
//  DESCRIPTION:   If the compiler cannot handle template specialisations
//                 for array types


namespace boost_no_array_type_specializations{

template< typename T > struct is_array
{
};

template< typename T, int N > struct is_array< T[N] >
{
};

int test()
{
   return 0;
}

}
