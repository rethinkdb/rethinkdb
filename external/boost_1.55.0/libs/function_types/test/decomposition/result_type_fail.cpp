
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/function_types/result_type.hpp>


namespace ft = boost::function_types;

class C; 

typedef ft::result_type<C>::type error_1;
typedef ft::result_type<char>::type error_2;
typedef ft::result_type<void>::type error_3;

