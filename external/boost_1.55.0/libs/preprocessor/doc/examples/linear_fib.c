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
# /* This example shows how BOOST_PP_WHILE() can be used for implementing macros. */
#
# include <stdio.h>
#
# include <boost/preprocessor/arithmetic/add.hpp>
# include <boost/preprocessor/arithmetic/sub.hpp>
# include <boost/preprocessor/comparison/less_equal.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/list/adt.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* First consider the following C implementation of Fibonacci. */

typedef struct linear_fib_state {
   int a0, a1, n;
} linear_fib_state;

static int linear_fib_c(linear_fib_state p) {
   return p.n;
}

static linear_fib_state linear_fib_f(linear_fib_state p) {
   linear_fib_state r = { p.a1, p.a0 + p.a1, p.n - 1 };
   return r;
}

static int linear_fib(int n) {
   linear_fib_state p = { 0, 1, n };
   while (linear_fib_c(p)) {
      p = linear_fib_f(p);
   }
   return p.a0;
}

# /* Then consider the following preprocessor implementation of Fibonacci. */
#
# define LINEAR_FIB(n) LINEAR_FIB_D(1, n)
# /* Since the macro is implemented using BOOST_PP_WHILE, the actual
#  * implementation takes a depth as a parameters so that it can be called
#  * inside a BOOST_PP_WHILE.  The above easy-to-use version simply uses 1
#  * as the depth and cannot be called inside a BOOST_PP_WHILE.
#  */
#
# define LINEAR_FIB_D(d, n) \
   BOOST_PP_TUPLE_ELEM(3, 0, BOOST_PP_WHILE_ ## d(LINEAR_FIB_C, LINEAR_FIB_F, (0, 1, n)))
# /*                   ^^^^                 ^^^^^           ^^            ^^   ^^^^^^^
#  *                    #1                   #2             #3            #3     #4
#  *
#  * 1) The state is a 3-element tuple.  After the iteration is finished, the first
#  *    element of the tuple is the result.
#  *
#  * 2) The WHILE primitive is "invoked" directly.  BOOST_PP_WHILE(D, ...)
#  *    can't be used because it would not be expanded by the preprocessor.
#  *
#  * 3) ???_C is the condition and ???_F is the iteration macro.
#  */
#
# define LINEAR_FIB_C(d, p) \
   /* p.n */ BOOST_PP_TUPLE_ELEM(3, 2, p) \
   /**/
#
# define LINEAR_FIB_F(d, p) \
   ( \
      /* p.a1 */ BOOST_PP_TUPLE_ELEM(3, 1, p), \
      /* p.a0 + p.a1 */ BOOST_PP_ADD_D(d, BOOST_PP_TUPLE_ELEM(3, 0, p), BOOST_PP_TUPLE_ELEM(3, 1, p)), \
                        /*          ^^ ^ \
                         * BOOST_PP_ADD() uses BOOST_PP_WHILE().  Therefore we \
                         * pass the recursion depth explicitly to BOOST_PP_ADD_D(). \
                         */ \
      /* p.n - 1 */ BOOST_PP_DEC(BOOST_PP_TUPLE_ELEM(3, 2, p)) \
   ) \
   /**/

int main() {
   printf("linear_fib(10) = %d\n", linear_fib(10));
   printf("LINEAR_FIB(10) = %d\n", LINEAR_FIB(10));
   return 0;
}
