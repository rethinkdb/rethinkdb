
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CHECK_INTEGRAL_CONSTANT_HPP
#define BOOST_CHECK_INTEGRAL_CONSTANT_HPP

#ifdef USE_UNIT_TEST
#include <boost/test/test_tools.hpp>
#else
#include "test.hpp"
#endif

namespace boost{
   namespace detail{

      /*
      macro:
      BOOST_CHECK_INTEGRAL_CONSTANT(expression, expected_value)

      expression:     an integral constant expression to check.
      expected_value: the value expected.
      */

      template <long test_value>
      struct integral_constant
      {
         static long value() { return test_value; }
      };

      template <class T, class U>
      bool tt_compare(T found, U expected)
      { return static_cast<U>(found) == expected; }

#define BOOST_CHECK_INTEGRAL_CONSTANT(expression, expected_value)\
   if(!::boost::detail::tt_compare(::boost::detail::integral_constant<(int)(expression)>::value(), (int)expression)){\
      BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression) << "\" had differing values depending upon whether it was used as an integral constant expression or not");\
   }else{\
      BOOST_CHECK_MESSAGE(true, "Validating Integral Constant Expression: \"" << BOOST_STRINGIZE(expression) << "\"");\
   }\
   if(!::boost::detail::tt_compare((int)expression, expected_value))\
      BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression) << "\" had an invalid value (found " << ::boost::detail::integral_constant<(int)(expression)>::value() << ", expected " << expected_value << ")" )

      /*
      macro:
      BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(expression, expected_value, alternate_value)

      expression:      an integral constant expression to check.
      expected_value:  the value expected.
      alternate_value: an alternative value that results is a warning not a failure if found.
      */

#define BOOST_CHECK_SOFT_INTEGRAL_CONSTANT(expression, expected_value, alternate_value)\
   if(!::boost::detail::tt_compare(::boost::detail::integral_constant<(int)(expression)>::value(), (int)expression)){\
      BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression) << "\" had differing values depending upon whether it was used as an integral constant expression or not");\
   }else{\
      BOOST_CHECK_MESSAGE(true, "Validating Integral Constant Expression: \"" << BOOST_STRINGIZE(expression) << "\"");\
   }\
   if(!::boost::detail::tt_compare((int)expression, expected_value))\
   {\
      if(!::boost::detail::tt_compare((int)expression, alternate_value))\
      {\
         BOOST_CHECK_MESSAGE(false, "The expression: \"" << BOOST_STRINGIZE(expression) << "\" had an invalid value (found " << ::boost::detail::integral_constant<(int)(expression)>::value() << ", expected " << expected_value << ")" );\
      }\
      else\
      {\
         BOOST_WARN_MESSAGE(false, "<note>The expression: \"" << BOOST_STRINGIZE(expression) << "\" did not have the value we wish it to have (found " << ::boost::detail::integral_constant<(int)(expression)>::value() << ", expected " << expected_value << ")</note>" );\
      }\
    }


   }//detail
}//boost


#endif





