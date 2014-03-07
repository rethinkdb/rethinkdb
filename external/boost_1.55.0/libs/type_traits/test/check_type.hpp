
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CHECK_TYPE_HPP
#define BOOST_CHECK_TYPE_HPP

#ifdef USE_UNIT_TEST
#include <boost/test/test_tools.hpp>
#else
#include "test.hpp"
#endif
#include <boost/type_traits/is_same.hpp>

/*
macro:
BOOST_CHECK_TYPE(type_expression, expected_type)

type_expression:  an expression that evaluates to a typename.
expected_value:   the type we expect to find.
*/

#ifdef __BORLANDC__
#pragma option -w-8008 -w-8066 -w-8019
#endif


#define BOOST_CHECK_TYPE(type_expression, expected_type)\
do{\
   if(!::boost::is_same< type_expression, expected_type >::value){\
   BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression)\
      << "\" did not have the expected type:\n\tevaluating:   boost::is_same<"\
      << BOOST_STRINGIZE(type_expression) << ", " << BOOST_STRINGIZE(expected_type)\
      << ">" << "\n\tfound:        "\
      << typeid(::boost::is_same< type_expression, expected_type >).name());\
   }else\
      BOOST_CHECK_MESSAGE(true, "Validating Type Expression: \""\
         << BOOST_STRINGIZE(expression) << "\"");\
}while(0)

#define BOOST_CHECK_TYPE3(type_expression, type_expression_suffix, expected_type)\
do{\
   if(!::boost::is_same< type_expression, type_expression_suffix, expected_type >::value){\
   BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression)\
      << "\" did not have the expected type:\n\tevaluating:   boost::is_same<"\
      << BOOST_STRINGIZE((type_expression, type_expression_suffix)) << ", " << BOOST_STRINGIZE(expected_type)\
      << ">" << "\n\tfound:        "\
      << typeid(::boost::is_same< type_expression, type_expression_suffix, expected_type >).name());\
   }else\
      BOOST_CHECK_MESSAGE(true, "Validating Type Expression: \""\
         << BOOST_STRINGIZE(expression) << "\"");\
}while(0)

#define BOOST_CHECK_TYPE4(type_expression, suffix1, suffix2, expected_type)\
do{\
   if(!::boost::is_same< type_expression, suffix1, suffix2, expected_type >::value){\
   BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression)\
      << "\" did not have the expected type:\n\tevaluating:   boost::is_same<"\
      << BOOST_STRINGIZE((type_expression, suffix1, suffix2)) << ", " << BOOST_STRINGIZE(expected_type)\
      << ">" << "\n\tfound:        "\
      << typeid(::boost::is_same< type_expression, suffix1, suffix2, expected_type >).name());\
   }else\
      BOOST_CHECK_MESSAGE(true, "Validating Type Expression: \""\
         << BOOST_STRINGIZE(expression) << "\"");\
}while(0)

#endif


