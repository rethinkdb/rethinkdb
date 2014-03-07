
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/function_types/function_arity.hpp>


namespace ft = boost::function_types;

class C;

typedef ft::function_arity<C>::type error_1;
typedef ft::function_arity<int>::type error_2;
typedef ft::function_arity<void>::type error_3;

