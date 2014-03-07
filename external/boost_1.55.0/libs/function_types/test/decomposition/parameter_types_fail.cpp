
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/function_types/parameter_types.hpp>


namespace ft = boost::function_types;

class C; 

typedef ft::parameter_types<C>::type error_1;
typedef ft::parameter_types<char>::type error_2;
typedef ft::parameter_types<void>::type error_3;

