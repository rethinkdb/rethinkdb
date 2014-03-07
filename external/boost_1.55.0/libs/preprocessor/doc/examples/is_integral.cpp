# /* Copyright (C) 2002
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
# /* This example demonstrates the usage of preprocessor lists for generating C++ code. */
#
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/list/at.hpp>
# include <boost/preprocessor/list/for_each_product.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/to_list.hpp>
#
# /* List of integral types.  (Strictly speaking, wchar_t should be on the list.) */
# define INTEGRAL_TYPES \
   BOOST_PP_TUPLE_TO_LIST( \
      9, (char, signed char, unsigned char, short, unsigned short, int, unsigned, long, unsigned long) \
   ) \
   /**/
#
# /* List of invokeable cv-qualifiers */
# define CV_QUALIFIERS \
   BOOST_PP_TUPLE_TO_LIST( \
      4, (BOOST_PP_EMPTY, const BOOST_PP_EMPTY, volatile BOOST_PP_EMPTY, const volatile BOOST_PP_EMPTY) \
   ) \
   /**/
#
# /* Template for testing whether a type is an integral type. */

template<class T> struct is_integral {
   enum { value = false };
};

# /* Macro for defining a specialization of is_integral<> template. */
# define IS_INTEGRAL_SPECIALIZATION(R, L) \
   template<> struct is_integral<BOOST_PP_TUPLE_ELEM(2, 0, L)() BOOST_PP_TUPLE_ELEM(2, 1, L)> { \
      enum { value = true }; \
   }; \
   /**/

BOOST_PP_LIST_FOR_EACH_PRODUCT(IS_INTEGRAL_SPECIALIZATION, 2, (CV_QUALIFIERS, INTEGRAL_TYPES))
