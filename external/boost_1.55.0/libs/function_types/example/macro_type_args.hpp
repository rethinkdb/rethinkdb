
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
//
// This example implements a utility to accept a type expression, that may
// contain commas to a macro.
//
// 
// Detailed description
// ====================
//
// Accepting a type as macro argument can cause problems if the type expression
// contains commas:
//
//    #define MY_MACRO(a_type)
//    ...
//    MY_MACRO(std::map<int,int>)  // ERROR (wrong number of macro arguments)
//
// This problem can be solved by pasing using a parenthesized type
//
//    MY_MACRO((std::map<int,int>) // OK
//
// but then there is no way to remove the parentheses in the macro argument
// with the preprocessor.
// We can, however, form a pointer to a function with a single argument (the
// parentheses become part of the type) and extract the argument with template
// metaprogramming:
//
//   // Inside the macro definition
//
//   typename mpl::front< parameter_types<void(*)a_type> >::type 
//
// This code snippet does not read too expressive so we use another macro
// to encapsulate the solution:
//
//   // Inside the macro definition
//
//   BOOST_EXAMPLE_MACRO_TYPE_ARGUMENT(a_type)
// 
// As a generalization of this technique we can accept a comma-separated list of
// types. Omitting the mpl::front invocation gives us an MPL-sequence.
//
// 
// Limitations
// ===========
//
// - only works for types that are valid function arguments
//
// Acknowledgments
// ===============
//
// Thanks go to Dave Abrahams for letting me know this technique.

#ifndef BOOST_EXAMPLE_MACRO_TYPE_ARGUMENT_HPP_INCLUDED
#define BOOST_EXAMPLE_MACRO_TYPE_ARGUMENT_HPP_INCLUDED

#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/front.hpp>

#define BOOST_EXAMPLE_MACRO_TYPE_ARGUMENT(parenthesized_type)                  \
    boost::mpl::front<                                                         \
        BOOST_EXAMPLE_MACRO_TYPE_LIST_ARGUMENT(parenthesized_type) >::type

#define BOOST_EXAMPLE_MACRO_TYPE_LIST_ARGUMENT(parenthesized_types)            \
  ::boost::function_types::parameter_types< void(*) parenthesized_types >


#endif

