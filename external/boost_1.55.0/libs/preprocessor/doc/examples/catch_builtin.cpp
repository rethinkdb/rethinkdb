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
# /* This example demonstrates the usage of lists and BOOST_PP_LIST_FOR_EACH(). */
#
# include <iostream>
# include <typeinfo>
#
# include <boost/preprocessor/list/for_each.hpp>
# include <boost/preprocessor/tuple/to_list.hpp>
#
# /* List of built-in types.  (Strictly speaking wchar_t should be on the list.) */
#
# define BUILTIN_TYPES \
   BOOST_PP_TUPLE_TO_LIST( \
      13, \
      ( \
         bool, \
         char, signed char, unsigned char, \
         unsigned short, short, \
         int, unsigned, \
         long, unsigned long, \
         float, \
         double, long double \
      ) \
   ) \
   /**/
#
# define CATCH(R, _, T) \
   catch (T t) { \
      std::cerr << "Caught an " << typeid(t).name() << " = " << t; \
   } \
   /**/

int main() {
   try {
      throw 10;
   }
   BOOST_PP_LIST_FOR_EACH(CATCH, _, BUILTIN_TYPES)
   return 0;
}
