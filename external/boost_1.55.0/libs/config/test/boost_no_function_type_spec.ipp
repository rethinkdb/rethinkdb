//  (C) Copyright John Maddock 2001.
//  (C) Copyright Aleksey Gurtovoy 2003.
//  (C) Copyright Alisdair Meredith 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_FUNCTION_TYPE_SPECIALIZATIONS
//  TITLE:         template specialisations of function types
//  DESCRIPTION:   If the compiler cannot handle template specialisations
//                 for function types


namespace boost_no_function_type_specializations{

template< typename T > struct is_function
{
};

struct X {};
enum   Y { value };

//  Tesst can declare specializations
typedef is_function< int( int ) >   scalar_types;
typedef is_function< X( X ) >       user_defined_type;
typedef is_function< int( Y ) >     check_enum;
typedef is_function< X( X, int ) >  multiple_arguments;

//  Partial specialization test
//  confirm const, volatile, pointers and references in args
template< typename X, typename Y, typename Z >
struct is_function< X( Y const &, volatile Z * ) >
{
};


int test()
{
   return 0;
}

}
